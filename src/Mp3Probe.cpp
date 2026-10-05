#include "Mp3Probe.h"

#include <cstring>

namespace mrm {

namespace {

constexpr uint32_t kSearch = 64 * 1024;
constexpr size_t kChunk = 512;
constexpr uint32_t kSameStream = 0xFFFE0C00; // sync, versao, camada e taxa
constexpr uint8_t kMaxTags = 4;              // ID3v2 repetida acontece em arquivo editado varias vezes
constexpr uint16_t kBitrateV1[16] = {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 0};
constexpr uint16_t kBitrateV2[16] = {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0};
constexpr uint16_t kRateV1[3] = {44100, 48000, 32000};

struct Frame {
    uint32_t len;
    uint32_t rate;
    uint16_t kbps;
    uint16_t samples; // por canal
    uint8_t channels;
    uint8_t side; // bytes de side info depois do cabecalho
};

// Leitura que aproveita o bloco ja lido (cada read no LittleFS custa ~0,5 ms).
struct Source {
    Mp3ReadAt readAt;
    void* ctx;
    uint32_t size;
    const uint8_t* win = nullptr;
    uint32_t winAt = 0;
    size_t winLen = 0;

    size_t get(uint32_t at, uint8_t* dst, size_t n) const {
        if (win && at >= winAt && at - winAt + n <= winLen) {
            memcpy(dst, win + (at - winAt), n);
            return n;
        }
        return readAt(ctx, at, dst, n);
    }
};

uint32_t be32(const uint8_t* p) {
    return uint32_t(p[0]) << 24 | uint32_t(p[1]) << 16 | uint32_t(p[2]) << 8 | p[3];
}

bool parse(uint32_t h, Frame& f) {
    const uint32_t version = (h >> 19) & 3; // 3 = MPEG-1, 2 = MPEG-2, 0 = MPEG-2.5
    const uint32_t bitrateIdx = (h >> 12) & 15;
    const uint32_t rateIdx = (h >> 10) & 3;
    if ((h >> 21) != 0x7FF || version == 1 || ((h >> 17) & 3) != 1)
        return false;
    if (bitrateIdx == 0 || bitrateIdx == 15 || rateIdx == 3)
        return false;
    const bool v1 = version == 3;
    f.rate = kRateV1[rateIdx] >> (v1 ? 0 : (version == 2 ? 1 : 2));
    f.kbps = v1 ? kBitrateV1[bitrateIdx] : kBitrateV2[bitrateIdx];
    f.samples = v1 ? 1152 : 576;
    f.channels = ((h >> 6) & 3) == 3 ? 1 : 2;
    f.side = v1 ? (f.channels == 1 ? 17 : 32) : (f.channels == 1 ? 9 : 17);
    f.len = (v1 ? 144u : 72u) * f.kbps * 1000 / f.rate + ((h >> 9) & 1);
    return true;
}

// O frame seguinte precisa ter a mesma versao, camada e taxa; um frame unico que fecha o arquivo vale.
bool confirmed(const Source& src, uint32_t at, uint32_t header, const Frame& f) {
    const uint32_t next = at + f.len;
    if (next == src.size)
        return true;
    uint8_t b[4];
    Frame g;
    if (next + 4 > src.size || src.get(next, b, 4) != 4)
        return false;
    const uint32_t h = be32(b);
    return parse(h, g) && ((h ^ header) & kSameStream) == 0;
}

// Frame Xing/Info ou VBRI no inicio: traz o total de frames (duracao exata em VBR) e nao e audio.
// Do Xing tambem saem a tabela de busca e o total de bytes.
bool vbrFrames(const Source& src, uint32_t at, const Frame& f, uint32_t& frames, Mp3Info& out) {
    constexpr size_t kXingMax = 4 + 32 + 4 + 4 + 4 + 4 + 100; // cabecalho, side info, "Xing", flags, frames, bytes, tabela
    uint8_t b[kXingMax];
    const size_t n = src.get(at, b, at + sizeof(b) <= src.size ? sizeof(b) : src.size - at);
    frames = 0;
    const size_t xing = 4u + f.side;
    if (n >= xing + 12 && (memcmp(b + xing, "Xing", 4) == 0 || memcmp(b + xing, "Info", 4) == 0)) {
        const uint32_t flags = be32(b + xing + 4);
        size_t p = xing + 8;
        if (flags & 1) {
            frames = be32(b + p);
            p += 4;
        }
        if ((flags & 2) && p + 4 <= n) {
            out.audioBytes = be32(b + p);
            p += 4;
        }
        if ((flags & 4) && p + sizeof(out.toc) <= n) {
            memcpy(out.toc, b + p, sizeof(out.toc));
            out.hasToc = true;
        }
        return true;
    }
    if (n >= 36 + 18 && memcmp(b + 36, "VBRI", 4) == 0) {
        frames = be32(b + 36 + 14);
        return true;
    }
    return false;
}

uint32_t afterTags(Mp3ReadAt readAt, void* ctx, uint32_t fileSize) {
    uint32_t off = 0;
    for (uint8_t i = 0; i < kMaxTags; ++i) {
        uint8_t b[10];
        if (readAt(ctx, off, b, sizeof(b)) != sizeof(b) || memcmp(b, "ID3", 3) != 0)
            return off;
        uint32_t size = 0;
        for (int k = 6; k < 10; ++k) {
            if (b[k] & 0x80)
                return fileSize; // tamanho nao e synchsafe: tag corrompida
            size = (size << 7) | b[k];
        }
        off += 10 + size + ((b[5] & 0x10) ? 10 : 0);
        if (off >= fileSize)
            return fileSize;
    }
    return off;
}

void fill(const Source& src, uint32_t at, const Frame& f, Mp3Info& out) {
    out.sampleRate = f.rate;
    out.channels = f.channels;
    out.kbps = f.kbps;
    out.dataOffset = at;
    out.firstFrame = at;
    uint32_t frames = 0;
    if (vbrFrames(src, at, f, frames, out))
        out.dataOffset = at + f.len;
    if (out.audioBytes <= out.dataOffset - at || out.audioBytes > src.size - at)
        out.audioBytes = src.size - at;
    if (frames) {
        out.durationMs = static_cast<uint32_t>(uint64_t(frames) * f.samples * 1000 / f.rate);
        // Em CBR a taxa media e exata e a tabela (passo de 1/256 do arquivo) so piora.
        const uint64_t byRate = uint64_t(out.audioBytes) * 8 / f.kbps;
        const uint64_t gap = byRate > out.durationMs ? byRate - out.durationMs : out.durationMs - byRate;
        if (gap * 500 < out.durationMs)
            out.hasToc = false;
        return;
    }
    out.hasToc = false; // tabela sem total de frames nao diz nada sobre o tempo
    uint32_t end = src.size;
    uint8_t tag[3];
    if (end >= out.dataOffset + 128 && src.get(end - 128, tag, 3) == 3 && memcmp(tag, "TAG", 3) == 0)
        end -= 128; // ID3v1
    out.durationMs = static_cast<uint32_t>(uint64_t(end - out.dataOffset) * 8 / f.kbps);
}

} // namespace

uint32_t mp3OffsetAt(const Mp3Info& info, uint32_t ms) {
    if (info.durationMs == 0 || ms == 0)
        return info.dataOffset;
    if (ms > info.durationMs)
        ms = info.durationMs;
    uint64_t at; // posicao desde firstFrame
    if (info.hasToc) {
        const uint32_t t = static_cast<uint32_t>(uint64_t(ms) * 10000 / info.durationMs); // centesimos de %
        const uint32_t i = t / 100 < 99 ? t / 100 : 99;
        const uint32_t lo = info.toc[i] * 100u;
        const uint32_t hi = i < 99 ? info.toc[i + 1] * 100u : 256u * 100u;
        const uint32_t x = hi >= lo ? lo + (hi - lo) * (t - i * 100) / 100 : lo; // x/256, vezes 100
        at = uint64_t(info.audioBytes) * x / (256 * 100);
    } else {
        const uint64_t data = info.audioBytes - (info.dataOffset - info.firstFrame);
        at = (info.dataOffset - info.firstFrame) + data * ms / info.durationMs;
    }
    const uint64_t pos = info.firstFrame + at;
    const uint64_t last = uint64_t(info.firstFrame) + info.audioBytes - 1;
    return static_cast<uint32_t>(pos < info.dataOffset ? info.dataOffset : pos > last ? last : pos);
}

bool probeMp3(Mp3ReadAt readAt, void* ctx, uint32_t fileSize, Mp3Info& out) {
    out = Mp3Info{};
    const uint32_t start = afterTags(readAt, ctx, fileSize);
    const uint32_t end = fileSize - start > kSearch ? start + kSearch : fileSize;
    uint8_t b[kChunk];
    Source src{readAt, ctx, fileSize};
    for (uint32_t base = start; base + 4 <= end;) {
        const size_t want = end - base < kChunk ? end - base : kChunk;
        const size_t n = readAt(ctx, base, b, want);
        if (n < 4)
            return false;
        src.win = b;
        src.winAt = base;
        src.winLen = n;
        for (size_t i = 0; i + 4 <= n; ++i) {
            Frame f;
            const uint32_t h = be32(b + i);
            if (parse(h, f) && confirmed(src, base + i, h, f)) {
                fill(src, base + i, f, out);
                return true;
            }
        }
        base += n - 3;
    }
    return false;
}

} // namespace mrm
