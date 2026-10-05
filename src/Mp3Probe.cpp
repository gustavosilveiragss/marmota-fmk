#include "Mp3Probe.h"

#include "Mp3Header.h"

#include <cstring>

namespace mrm {

namespace {

using namespace mp3header;

constexpr uint32_t kSearchBytes = 64 * 1024;
constexpr size_t kChunkBytes = 512;
constexpr uint8_t kMaxTags = 4; // ID3v2 repetida acontece em arquivo editado várias vezes
constexpr size_t kFrameHeaderBytes = 4;
constexpr size_t kId3HeaderBytes = 10;
constexpr size_t kId3FooterBytes = 10;
constexpr uint8_t kId3FooterFlag = 0x10;
constexpr uint8_t kId3SizeAt = 6;
constexpr uint32_t kId3v1Bytes = 128;
constexpr size_t kId3v1TagBytes = 3; // "TAG"

constexpr uint8_t kSideInfoBytes[2][2] = {{9, 17}, {17, 32}}; // [MPEG-1][estéreo]

constexpr size_t kXingTagBytes = 4;
constexpr size_t kXingFieldBytes = 4;
constexpr uint32_t kXingHasFrames = 1;
constexpr uint32_t kXingHasBytes = 2;
constexpr uint32_t kXingHasToc = 4;
constexpr size_t kXingMinBytes = kXingTagBytes + 2 * kXingFieldBytes;
// cabeçalho, side info máximo, "Xing", flags, frames, bytes, tabela
constexpr size_t kXingMaxBytes = kFrameHeaderBytes + 32 + kXingTagBytes + 3 * kXingFieldBytes + 100;
constexpr size_t kVbriTagAt = 36; // fixo: depois do cabeçalho e de 32 bytes de side info
constexpr size_t kVbriFramesAt = kVbriTagAt + 14;
constexpr size_t kVbriMinBytes = kVbriTagAt + 18;

constexpr uint32_t kTocEntries = 100;
constexpr uint32_t kTocScale = 256;          // cada entrada e a posição em 256avos do arquivo
constexpr uint32_t kPermyriadPerEntry = 100; // cada entrada cobre 1% do tempo
constexpr uint32_t kPermyriad = 10000;
constexpr uint32_t kCbrGapDivisor = 500; // diferença de duração abaixo de 0,2% = CBR

struct Frame {
    uint32_t len;
    uint32_t rate;
    uint16_t kbps;
    uint16_t samples; // por canal
    uint8_t channels;
    uint8_t side; // bytes de side info depois do cabeçalho
};

// Leitura que aproveita o bloco já lido (cada read no LittleFS custa ~0,5 ms).
struct Source {
    Mp3ReadAt readAt;
    void* ctx;
    uint32_t size;
    const uint8_t* win = nullptr;
    uint32_t winAt = 0;
    size_t winLen = 0;

    size_t get(uint32_t offset, uint8_t* dst, size_t count) const {
        if (win && offset >= winAt && offset - winAt + count <= winLen) {
            memcpy(dst, win + (offset - winAt), count);
            return count;
        }

        return readAt(ctx, offset, dst, count);
    }
};

uint32_t be32(const uint8_t* bytes) {
    return uint32_t(bytes[0]) << 24 | uint32_t(bytes[1]) << 16 | uint32_t(bytes[2]) << 8 | bytes[3];
}

bool parse(uint32_t header, Frame& frame) {
    if (!isValidLayer3(header))
        return false;

    const bool mpeg1 = isMpeg1(header);
    frame.rate = sampleRateOf(header);
    frame.kbps = static_cast<uint16_t>(kbpsOf(header));

    frame.samples = mpeg1 ? kSamplesMpeg1 : kSamplesMpeg2;
    frame.channels = channelModeOf(header) == kChannelModeMono ? 1 : 2;
    frame.side = kSideInfoBytes[mpeg1][frame.channels == 2];
    frame.len = frameBytesOf(header);
    return true;
}

// O frame seguinte precisa ter a mesma versão, camada e taxa; um frame único que fecha o arquivo vale.
bool confirmed(const Source& src, uint32_t offset, uint32_t header, const Frame& frame) {
    const uint32_t next = offset + frame.len;
    if (next == src.size)
        return true;

    uint8_t raw[kFrameHeaderBytes];
    Frame following;
    if (next + sizeof(raw) > src.size || src.get(next, raw, sizeof(raw)) != sizeof(raw))
        return false;

    const uint32_t nextHeader = be32(raw);
    return parse(nextHeader, following) && ((nextHeader ^ header) & kSameStreamMask) == 0;
}

// Xing/Info: total de frames, total de bytes e tabela de busca, nesta ordem e conforme as flags.
inline bool xingFrames(const uint8_t* raw, size_t count, size_t tagAt, uint32_t& frames, Mp3Info& out) {
    if (count < tagAt + kXingMinBytes || (memcmp(raw + tagAt, "Xing", kXingTagBytes) != 0 &&
        memcmp(raw + tagAt, "Info", kXingTagBytes) != 0))
        return false;

    const uint32_t flags = be32(raw + tagAt + kXingTagBytes);
    size_t pos = tagAt + kXingTagBytes + kXingFieldBytes; // depois de "Xing" e das flags
    if (flags & kXingHasFrames) {
        frames = be32(raw + pos);
        pos += kXingFieldBytes;
    }

    if ((flags & kXingHasBytes) && pos + kXingFieldBytes <= count) {
        out.audioBytes = be32(raw + pos);
        pos += kXingFieldBytes;
    }

    if ((flags & kXingHasToc) && pos + sizeof(out.toc) <= count) {
        memcpy(out.toc, raw + pos, sizeof(out.toc));
        out.hasToc = true;
    }

    return true;
}

// Frame Xing/Info ou VBRI no início: traz o total de frames (duração exata em VBR) e não é áudio.
bool vbrFrames(const Source& src, uint32_t offset, const Frame& frame, uint32_t& frames, Mp3Info& out) {
    uint8_t raw[kXingMaxBytes];
    const size_t count = src.get(offset, raw, offset + sizeof(raw) <= src.size ? sizeof(raw) : src.size - offset);
    frames = 0;
    if (xingFrames(raw, count, kFrameHeaderBytes + frame.side, frames, out))
        return true;

    if (count >= kVbriMinBytes && memcmp(raw + kVbriTagAt, "VBRI", 4) == 0) {
        frames = be32(raw + kVbriFramesAt);
        return true;
    }

    return false;
}

uint32_t afterTags(Mp3ReadAt readAt, void* ctx, uint32_t fileSize) {
    uint32_t off = 0;
    for (uint8_t i = 0; i < kMaxTags; ++i) {
        uint8_t raw[kId3HeaderBytes];
        if (readAt(ctx, off, raw, sizeof(raw)) != sizeof(raw) || memcmp(raw, "ID3", 3) != 0)
            return off;

        uint32_t size = 0;
        for (size_t k = kId3SizeAt; k < kId3HeaderBytes; ++k) {
            if (raw[k] & 0x80)
                return fileSize; // tamanho não e synchsafe: tag corrompida
            size = (size << 7) | raw[k];
        }

        off += kId3HeaderBytes + size + ((raw[5] & kId3FooterFlag) ? kId3FooterBytes : 0);
        if (off >= fileSize)
            return fileSize;
    }

    return off;
}

// Duração exata pelo total de frames. Em CBR a taxa média é exata e a tabela (passo de 1/256 do
// arquivo) só piora, então ela é descartada.
inline void durationFromFrames(uint32_t frames, const Frame& frame, Mp3Info& out) {
    out.durationMs = static_cast<uint32_t>(uint64_t(frames) * frame.samples * 1000 / frame.rate);
    const uint64_t byRate = uint64_t(out.audioBytes) * 8 / frame.kbps;
    const uint64_t gap = byRate > out.durationMs ? byRate - out.durationMs : out.durationMs - byRate;
    if (gap * kCbrGapDivisor < out.durationMs)
        out.hasToc = false;
}

// Sem total de frames: tamanho do áudio (sem o ID3v1 final) x 8 / bitrate.
inline void durationFromSize(const Source& src, const Frame& frame, Mp3Info& out) {
    out.hasToc = false; // tabela sem total de frames não diz nada sobre o tempo
    uint32_t end = src.size;
    uint8_t tag[kId3v1TagBytes];
    if (end >= out.dataOffset + kId3v1Bytes && src.get(end - kId3v1Bytes, tag, sizeof(tag)) == sizeof(tag) &&
        memcmp(tag, "TAG", sizeof(tag)) == 0)
        end -= kId3v1Bytes;
    out.durationMs = static_cast<uint32_t>(uint64_t(end - out.dataOffset) * 8 / frame.kbps);
}

void fill(const Source& src, uint32_t offset, const Frame& frame, Mp3Info& out) {
    out.sampleRate = frame.rate;
    out.channels = frame.channels;
    out.kbps = frame.kbps;
    out.dataOffset = offset;
    out.firstFrame = offset;

    uint32_t frames = 0;
    if (vbrFrames(src, offset, frame, frames, out))
        out.dataOffset = offset + frame.len;

    if (out.audioBytes <= out.dataOffset - offset || out.audioBytes > src.size - offset)
        out.audioBytes = src.size - offset;

    if (frames)
        durationFromFrames(frames, frame, out);
    else
        durationFromSize(src, frame, out);
}

// Posição desde firstFrame pela tabela do Xing, interpolando entre duas entradas.
inline uint64_t tocOffset(const Mp3Info& info, uint32_t ms) {
    const uint32_t permyriad = static_cast<uint32_t>(uint64_t(ms) * kPermyriad / info.durationMs);
    const uint32_t entry = permyriad / kPermyriadPerEntry < kTocEntries - 1 ?
            permyriad / kPermyriadPerEntry : kTocEntries - 1;
    const uint32_t low = info.toc[entry] * kPermyriadPerEntry;
    const uint32_t high = entry < kTocEntries - 1 ? info.toc[entry + 1] * kPermyriadPerEntry
            : kTocScale * kPermyriadPerEntry;

    const uint32_t scaled = high >= low ? low + (high - low) *
            (permyriad - entry * kPermyriadPerEntry) / kPermyriadPerEntry : low;
    return uint64_t(info.audioBytes) * scaled / (kTocScale * kPermyriadPerEntry);
}

// Posição desde firstFrame pela taxa média: o frame Xing fica fora da regra de três.
inline uint64_t linearOffset(const Mp3Info& info, uint32_t ms) {
    const uint64_t skipped = info.dataOffset - info.firstFrame;
    const uint64_t data = info.audioBytes - skipped;
    return skipped + data * ms / info.durationMs;
}

} // namespace

uint32_t mp3OffsetAt(const Mp3Info& info, uint32_t ms) {
    if (info.durationMs == 0 || ms == 0)
        return info.dataOffset;

    if (ms > info.durationMs)
        ms = info.durationMs;
    const uint64_t offset = info.hasToc ? tocOffset(info, ms) : linearOffset(info, ms);
    const uint64_t pos = info.firstFrame + offset;
    const uint64_t last = uint64_t(info.firstFrame) + info.audioBytes - 1;
    return static_cast<uint32_t>(pos < info.dataOffset ? info.dataOffset : pos > last ? last
                                                                                      : pos);
}

bool probeMp3(Mp3ReadAt readAt, void* ctx, uint32_t fileSize, Mp3Info& out) {
    out = Mp3Info{};

    const uint32_t start = afterTags(readAt, ctx, fileSize);
    const uint32_t end = fileSize - start > kSearchBytes ? start + kSearchBytes : fileSize;

    uint8_t chunk[kChunkBytes];
    Source src{readAt, ctx, fileSize};
    for (uint32_t base = start; base + kFrameHeaderBytes <= end;) {
        const size_t want = end - base < kChunkBytes ? end - base : kChunkBytes;
        const size_t count = readAt(ctx, base, chunk, want);

        if (count < kFrameHeaderBytes)
            return false;

        src.win = chunk;
        src.winAt = base;
        src.winLen = count;

        for (size_t i = 0; i + kFrameHeaderBytes <= count; ++i) {
            Frame frame;
            const uint32_t header = be32(chunk + i);
            if (parse(header, frame) && confirmed(src, base + i, header, frame)) {
                fill(src, base + i, frame, out);
                return true;
            }
        }

        base += count - (kFrameHeaderBytes - 1);
    }

    return false;
}

} // namespace mrm
