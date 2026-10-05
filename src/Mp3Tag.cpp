#include "Mp3Tag.h"

#include <cstring>

namespace mrm {

namespace {

constexpr uint8_t kMaxFrames = 64;          // cabeçalhos percorridos até achar o TPE1
constexpr size_t kRawBytes = 128;           // bytes do frame lidos: cobre kMaxArtistLen em UTF-16
constexpr size_t kId3TagHeaderBytes = 10;   // "ID3", versão, flags, tamanho
constexpr size_t kId3FrameHeaderBytes = 10; // id, tamanho, 2 bytes de flags
constexpr size_t kTagSizeAt = 6;
constexpr size_t kFrameSizeAt = 4;
constexpr size_t kExtendedSizeBytes = 4;
constexpr uint8_t kTagFlagUnsync = 0x80;
constexpr uint8_t kTagFlagExtended = 0x40;
constexpr uint8_t kV24FrameCompressedOrEncrypted = 0x0C;
constexpr uint8_t kV24FrameGrouping = 0x40;
constexpr uint8_t kV24FrameDataLength = 0x01;
constexpr uint8_t kV24FrameUnsync = 0x02;
constexpr uint8_t kV23FrameCompressedOrEncrypted = 0xC0;
constexpr uint8_t kV23FrameGrouping = 0x20;
constexpr size_t kDataLengthBytes = 4;
constexpr size_t kId3v1Bytes = 128;
constexpr size_t kId3v1ArtistAt = 33;
constexpr size_t kId3v1ArtistBytes = 30;
constexpr size_t kId3v1ReadBytes = kId3v1ArtistAt + kId3v1ArtistBytes; // "TAG", título (30), artista (30)

constexpr uint32_t kBom = 0xFEFF;
constexpr uint32_t kReplacement = 0xFFFD;
constexpr uint32_t kBeyondLatin1 = 0x10000;
constexpr uint32_t kLatin1Max = 0xFF;
constexpr uint32_t kC1First = 0x7F;
constexpr uint32_t kC1Last = 0xA0;
constexpr uint32_t kHighSurrogate = 0xD800;
constexpr uint32_t kLowSurrogate = 0xDC00;
constexpr uint32_t kSurrogateEnd = 0xE000;
constexpr uint32_t kSurrogateMask = 0xFC00;

enum Encoding : uint8_t { kLatin1,
                          kUtf16Bom,
                          kUtf16Be,
                          kUtf8,
                          kEncodingCount };

struct Punctuation {
    uint16_t from;
    char ascii;
};
// Tipográficos que o OLED não tem viram o ASCII mais próximo.
constexpr Punctuation kPunctuationMap[] = {
    {0x2018, '\''},
    {0x2019, '\''},
    {0x201C, '"'},
    {0x201D, '"'},
    {0x2013, '-'},
    {0x2014, '-'},
};

uint32_t mapPunctuation(uint32_t codePoint) {
    for (const Punctuation& entry : kPunctuationMap)
        if (entry.from == codePoint)
            return static_cast<uint32_t>(entry.ascii);
    return codePoint;
}

// Acumula code points como UTF-8 de Latin-1: o glifo que a fonte não tem vira um único '?'.
class Utf8Out {
public:
    Utf8Out(char* buf, size_t cap)
        : buf_(buf)
        , cap_(cap) {
        buf_[0] = '\0';
    }

    void put(uint32_t codePoint) {
        if (codePoint == kBom || codePoint < ' ' || (codePoint >= kC1First && codePoint < kC1Last) ||
            (codePoint == ' ' && len_ == 0))
            return;

        codePoint = mapPunctuation(codePoint);
        const bool lost = codePoint > kLatin1Max;
        if (lost && lostLast_)
            return;

        lostLast_ = lost;
        if (lost)
            codePoint = '?';

        const size_t need = codePoint < 0x80 ? 1 : 2;
        if (full_ || len_ + need > cap_) {
            full_ = true;
            return;
        }

        if (need == 1) {
            buf_[len_++] = static_cast<char>(codePoint);
        } else {
            buf_[len_++] = static_cast<char>(0xC0 | (codePoint >> 6));
            buf_[len_++] = static_cast<char>(0x80 | (codePoint & 0x3F));
        }

        buf_[len_] = '\0';
    }

    bool finish() {
        while (len_ > 0 && buf_[len_ - 1] == ' ')
            buf_[--len_] = '\0';
        return len_ > 0;
    }

private:
    char* buf_;
    size_t cap_;
    size_t len_ = 0;
    bool lostLast_ = false;
    bool full_ = false;
};

void decodeLatin1(const uint8_t* bytes, size_t count, Utf8Out& out) {
    for (size_t i = 0; i < count && bytes[i]; ++i)
        out.put(bytes[i]);
}

void decodeUtf8(const uint8_t* bytes, size_t count, Utf8Out& out) {
    for (size_t i = 0; i < count && bytes[i];) {
        const uint8_t byte = bytes[i];
        const size_t len = byte < 0x80 ? 1 : (byte & 0xE0) == 0xC0 ? 2
                                         : (byte & 0xF0) == 0xE0   ? 3
                                         : (byte & 0xF8) == 0xF0   ? 4
                                                                   : 0;

        if (len == 0) {
            out.put(kReplacement);
            ++i;
            continue;
        }

        if (i + len > count)
            return; // sequência cortada pelo fim do texto
        uint32_t codePoint = len == 1 ? byte : byte & (0x7F >> len);
        for (size_t k = 1; k < len; ++k)
            codePoint = (codePoint << 6) | (bytes[i + k] & 0x3F);
        out.put(codePoint);
        i += len;
    }
}

void decodeUtf16(const uint8_t* bytes, size_t count, bool bigEndian, Utf8Out& out) {
    size_t i = 0;
    if (count >= 2 && bytes[0] == 0xFF && bytes[1] == 0xFE) { // o BOM manda; sem BOM vale o padrão da codificação
        bigEndian = false;
        i = 2;
    } else if (count >= 2 && bytes[0] == 0xFE && bytes[1] == 0xFF) {
        bigEndian = true;
        i = 2;
    }

    auto unit = [&](size_t at) { return bigEndian ?
        (bytes[at] << 8) | bytes[at + 1] : (bytes[at + 1] << 8) | bytes[at]; };
    for (; i + 2 <= count; i += 2) {
        uint32_t u = unit(i);
        if (u == 0)
            return;

        if (u >= kHighSurrogate && u < kLowSurrogate && i + 4 <= count &&
            (unit(i + 2) & kSurrogateMask) == kLowSurrogate) {
            u = kBeyondLatin1; // fora do Latin-1 de qualquer jeito
            i += 2;
        } else if (u >= kHighSurrogate && u < kSurrogateEnd) {
            u = kReplacement;
        }

        out.put(u);
    }
}

// Valor: byte de codificação e texto até o primeiro terminador (o primeiro de vários artistas).
bool decodeText(const uint8_t* bytes, size_t count, Utf8Out& out) {
    if (count < 2 || bytes[0] >= kEncodingCount)
        return false;

    switch (bytes[0]) {
    case kLatin1:
        decodeLatin1(bytes + 1, count - 1, out);
        break;

    case kUtf16Bom:
        decodeUtf16(bytes + 1, count - 1, false, out);
        break;

    case kUtf16Be:
        decodeUtf16(bytes + 1, count - 1, true, out);
        break;

    default:
        decodeUtf8(bytes + 1, count - 1, out);
        break;
    }

    return out.finish();
}

bool synchsafe(const uint8_t* byte, uint32_t& out) {
    out = 0;
    for (int k = 0; k < 4; ++k) {
        if (byte[k] & 0x80)
            return false;
        out = (out << 7) | byte[k];
    }

    return true;
}

// Tamanho de tag ou frame: synchsafe na 2.4, inteiro comum na 2.3.
bool sizeField(uint8_t version, const uint8_t* byte, uint32_t& out) {
    if (version == 4)
        return synchsafe(byte, out);
    out = uint32_t(byte[0]) << 24 | uint32_t(byte[1]) << 16 | uint32_t(byte[2]) << 8 | byte[3];
    return true;
}

// Desfaz a unsynchronisation (FF 00 -> FF) no próprio buffer.
size_t deUnsync(uint8_t* bytes, size_t count) {
    size_t w = 0;
    for (size_t r = 0; r < count; ++r) {
        bytes[w++] = bytes[r];
        if (bytes[r] == 0xFF && r + 1 < count && bytes[r + 1] == 0)
            ++r;
    }

    return w;
}

// Dados do frame começam depois dos bytes opcionais de agrupamento e tamanho; false se o frame vem
// comprimido ou cifrado. flags e o segundo byte de flags do cabeçalho do frame.
bool dataStart(uint8_t version, uint8_t flags, uint8_t& skip, bool& unsync) {
    skip = 0;
    unsync = false;
    if (version == 4) {
        if (flags & kV24FrameCompressedOrEncrypted)

            return false;
        skip = ((flags & kV24FrameGrouping) ? 1 : 0) + ((flags & kV24FrameDataLength) ? kDataLengthBytes : 0);
        unsync = flags & kV24FrameUnsync;
    } else {
        if (flags & kV23FrameCompressedOrEncrypted)
            return false;

        skip = (flags & kV23FrameGrouping) ? 1 : 0;
    }

    return true;
}

struct TagContext {
    Mp3ReadAt readAt;
    void* ctx;
    uint8_t version;
    uint8_t tagFlags;
};

struct FrameRef {
    uint32_t at; // início dos dados
    uint32_t size;
    uint8_t flags;
};

inline bool frameArtist(const TagContext& tag, const FrameRef& frame, char* out) {
    uint8_t skip;
    bool unsync;
    if (!dataStart(tag.version, frame.flags, skip, unsync) || frame.size <= skip)
        return false;

    uint8_t raw[kRawBytes];
    size_t count = tag.readAt(tag.ctx, frame.at + skip, raw,
        frame.size - skip < kRawBytes ? frame.size - skip : kRawBytes);
    if (unsync || (tag.tagFlags & kTagFlagUnsync))
        count = deUnsync(raw, count);

    Utf8Out text(out, kMaxArtistLen);
    return decodeText(raw, count, text);
}

// Avança pos além do cabeçalho estendido; false se ele não cabe na tag.
inline bool skipExtendedHeader(const TagContext& tag, uint32_t end, uint32_t& pos) {
    uint8_t raw[kExtendedSizeBytes];
    uint32_t size;
    if (tag.readAt(tag.ctx, pos, raw, sizeof(raw)) != sizeof(raw) || !sizeField(tag.version, raw, size))
        return false;

    size = tag.version == 4 ? size : size + kExtendedSizeBytes; // na 2.4 o tamanho conta o próprio campo
    if (size > end - pos)
        return false;
    pos += size;
    return true;
}

bool v2Artist(Mp3ReadAt readAt, void* ctx, uint32_t fileSize, char* out) {
    uint8_t head[kId3TagHeaderBytes];
    uint32_t tagSize;
    if (readAt(ctx, 0, head, sizeof(head)) != sizeof(head) ||
        memcmp(head, "ID3", 3) != 0 || (head[3] != 3 && head[3] != 4) ||
        !synchsafe(head + kTagSizeAt, tagSize))
        return false;

    const TagContext tag{readAt, ctx, head[3], head[5]};
    const uint32_t end = tagSize + kId3TagHeaderBytes < fileSize ? tagSize + kId3TagHeaderBytes : fileSize;
    uint32_t pos = kId3TagHeaderBytes;
    if ((tag.tagFlags & kTagFlagExtended) && !skipExtendedHeader(tag, end, pos))
        return false;

    for (uint8_t i = 0; i < kMaxFrames && pos + kId3FrameHeaderBytes <= end; ++i) {
        uint8_t raw[kId3FrameHeaderBytes];
        uint32_t size;
        if (readAt(ctx, pos, raw, sizeof(raw)) != sizeof(raw) || raw[0] == 0) // 0 = padding
            return false;

        if (!sizeField(tag.version, raw + kFrameSizeAt, size))
            return false;
        pos += kId3FrameHeaderBytes;
        if (size > end - pos)
            return false;

        if (memcmp(raw, "TPE1", 4) == 0)
            return frameArtist(tag, FrameRef{pos, size, raw[9]}, out);
        pos += size;
    }

    return false;
}

bool v1Artist(Mp3ReadAt readAt, void* ctx, uint32_t fileSize, char* out) {
    uint8_t raw[kId3v1ReadBytes];
    if (fileSize < kId3v1Bytes ||
        readAt(ctx, fileSize - kId3v1Bytes, raw, sizeof(raw)) != sizeof(raw) || memcmp(raw, "TAG", 3) != 0)
        return false;
    Utf8Out text(out, kMaxArtistLen);
    decodeLatin1(raw + kId3v1ArtistAt, kId3v1ArtistBytes, text);
    return text.finish();
}

} // namespace

bool readMp3Artist(Mp3ReadAt readAt, void* ctx, uint32_t fileSize, char* out) {
    out[0] = '\0';
    if (v2Artist(readAt, ctx, fileSize, out))
        return true;
    out[0] = '\0';
    return v1Artist(readAt, ctx, fileSize, out);
}

} // namespace mrm
