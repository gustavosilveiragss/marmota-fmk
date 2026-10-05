#include "Mp3Tag.h"

#include <cstring>

namespace mrm {

namespace {

constexpr uint8_t kMaxFrames = 64; // cabecalhos percorridos ate achar o TPE1
constexpr size_t kRaw = 128;       // bytes do frame lidos: cobre kMaxArtistLen em UTF-16

// Acumula code points como UTF-8 de Latin-1: o glifo que a fonte nao tem vira um unico '?'.
class Utf8Out {
public:
    Utf8Out(char* buf, size_t cap)
        : buf_(buf)
        , cap_(cap) {
        buf_[0] = '\0';
    }

    void put(uint32_t cp) {
        if (cp == 0xFEFF || cp < 0x20 || (cp >= 0x7F && cp < 0xA0) || (cp == ' ' && len_ == 0))
            return;
        if (cp == 0x2018 || cp == 0x2019)
            cp = '\'';
        else if (cp == 0x201C || cp == 0x201D)
            cp = '"';
        else if (cp == 0x2013 || cp == 0x2014)
            cp = '-';
        const bool lost = cp > 0xFF;
        if (lost && lostLast_)
            return;
        lostLast_ = lost;
        if (lost)
            cp = '?';
        const size_t need = cp < 0x80 ? 1 : 2;
        if (full_ || len_ + need > cap_) {
            full_ = true;
            return;
        }
        if (need == 1) {
            buf_[len_++] = static_cast<char>(cp);
        } else {
            buf_[len_++] = static_cast<char>(0xC0 | (cp >> 6));
            buf_[len_++] = static_cast<char>(0x80 | (cp & 0x3F));
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

void decodeLatin1(const uint8_t* p, size_t n, Utf8Out& out) {
    for (size_t i = 0; i < n && p[i]; ++i)
        out.put(p[i]);
}

void decodeUtf8(const uint8_t* p, size_t n, Utf8Out& out) {
    for (size_t i = 0; i < n && p[i];) {
        const uint8_t b = p[i];
        const size_t len = b < 0x80 ? 1 : (b & 0xE0) == 0xC0 ? 2 : (b & 0xF0) == 0xE0 ? 3 : (b & 0xF8) == 0xF0 ? 4 : 0;
        if (len == 0) {
            out.put(0xFFFD);
            ++i;
            continue;
        }
        if (i + len > n)
            return; // sequencia cortada pelo fim do texto
        uint32_t cp = len == 1 ? b : b & (0x7F >> len);
        for (size_t k = 1; k < len; ++k)
            cp = (cp << 6) | (p[i + k] & 0x3F);
        out.put(cp);
        i += len;
    }
}

void decodeUtf16(const uint8_t* p, size_t n, bool bigEndian, Utf8Out& out) {
    size_t i = 0;
    if (n >= 2 && p[0] == 0xFF && p[1] == 0xFE) { // o BOM manda; sem BOM vale o padrao da codificacao
        bigEndian = false;
        i = 2;
    } else if (n >= 2 && p[0] == 0xFE && p[1] == 0xFF) {
        bigEndian = true;
        i = 2;
    }
    auto unit = [&](size_t at) { return bigEndian ? (p[at] << 8) | p[at + 1] : (p[at + 1] << 8) | p[at]; };
    for (; i + 2 <= n; i += 2) {
        uint32_t u = unit(i);
        if (u == 0)
            return;
        if (u >= 0xD800 && u < 0xDC00 && i + 4 <= n && (unit(i + 2) & 0xFC00) == 0xDC00) {
            u = 0x10000; // fora do Latin-1 de qualquer jeito
            i += 2;
        } else if (u >= 0xD800 && u < 0xE000) {
            u = 0xFFFD;
        }
        out.put(u);
    }
}

// Valor: byte de codificacao e texto ate o primeiro terminador (o primeiro de varios artistas).
bool decodeText(const uint8_t* p, size_t n, Utf8Out& out) {
    if (n < 2 || p[0] > 3)
        return false;
    switch (p[0]) {
    case 0: decodeLatin1(p + 1, n - 1, out); break;
    case 1: decodeUtf16(p + 1, n - 1, false, out); break;
    case 2: decodeUtf16(p + 1, n - 1, true, out); break;
    default: decodeUtf8(p + 1, n - 1, out); break;
    }
    return out.finish();
}

bool synchsafe(const uint8_t* b, uint32_t& out) {
    out = 0;
    for (int k = 0; k < 4; ++k) {
        if (b[k] & 0x80)
            return false;
        out = (out << 7) | b[k];
    }
    return true;
}

// Tamanho de tag ou frame: synchsafe na 2.4, inteiro comum na 2.3.
bool sizeField(uint8_t version, const uint8_t* b, uint32_t& out) {
    if (version == 4)
        return synchsafe(b, out);
    out = uint32_t(b[0]) << 24 | uint32_t(b[1]) << 16 | uint32_t(b[2]) << 8 | b[3];
    return true;
}

// Desfaz a unsynchronisation (FF 00 -> FF) no proprio buffer.
size_t deUnsync(uint8_t* p, size_t n) {
    size_t w = 0;
    for (size_t r = 0; r < n; ++r) {
        p[w++] = p[r];
        if (p[r] == 0xFF && r + 1 < n && p[r + 1] == 0)
            ++r;
    }
    return w;
}

// Dados do frame comecam depois dos bytes opcionais de agrupamento e tamanho; false se o frame vem
// comprimido ou cifrado. flags e o segundo byte de flags do cabecalho do frame.
bool dataStart(uint8_t version, uint8_t flags, uint8_t& skip, bool& unsync) {
    skip = 0;
    unsync = false;
    if (version == 4) {
        if (flags & 0x0C)
            return false;
        skip = ((flags & 0x40) ? 1 : 0) + ((flags & 0x01) ? 4 : 0);
        unsync = flags & 0x02;
    } else {
        if (flags & 0xC0)
            return false;
        skip = (flags & 0x20) ? 1 : 0;
    }
    return true;
}

bool frameArtist(Mp3ReadAt readAt, void* ctx, uint32_t at, uint32_t size, uint8_t version, uint8_t flags, bool tagUnsync, char* out) {
    uint8_t skip;
    bool unsync;
    if (!dataStart(version, flags, skip, unsync) || size <= skip)
        return false;
    uint8_t raw[kRaw];
    size_t n = readAt(ctx, at + skip, raw, size - skip < kRaw ? size - skip : kRaw);
    if (unsync || tagUnsync)
        n = deUnsync(raw, n);
    Utf8Out text(out, kMaxArtistLen);
    return decodeText(raw, n, text);
}

bool v2Artist(Mp3ReadAt readAt, void* ctx, uint32_t fileSize, char* out) {
    uint8_t h[10];
    uint32_t tagSize;
    if (readAt(ctx, 0, h, sizeof(h)) != sizeof(h) || memcmp(h, "ID3", 3) != 0 || (h[3] != 3 && h[3] != 4) || !synchsafe(h + 6, tagSize))
        return false;
    const uint8_t version = h[3];
    const uint32_t end = tagSize + 10 < fileSize ? tagSize + 10 : fileSize;
    uint32_t pos = 10;
    if (h[5] & 0x40) { // cabecalho estendido: pula
        uint8_t e[4];
        uint32_t size;
        if (readAt(ctx, pos, e, sizeof(e)) != sizeof(e))
            return false;
        if (!sizeField(version, e, size))
            return false;
        size = version == 4 ? size : size + 4; // na 2.4 o tamanho conta o proprio campo
        if (size > end - pos)
            return false;
        pos += size;
    }
    for (uint8_t i = 0; i < kMaxFrames && pos + 10 <= end; ++i) {
        uint8_t f[10];
        uint32_t size;
        if (readAt(ctx, pos, f, sizeof(f)) != sizeof(f) || f[0] == 0) // 0 = padding
            return false;
        if (!sizeField(version, f + 4, size))
            return false;
        pos += 10;
        if (size > end - pos)
            return false;
        if (memcmp(f, "TPE1", 4) == 0)
            return frameArtist(readAt, ctx, pos, size, version, f[9], h[5] & 0x80, out);
        pos += size;
    }
    return false;
}

bool v1Artist(Mp3ReadAt readAt, void* ctx, uint32_t fileSize, char* out) {
    uint8_t b[63]; // "TAG", titulo (30), artista (30)
    if (fileSize < 128 || readAt(ctx, fileSize - 128, b, sizeof(b)) != sizeof(b) || memcmp(b, "TAG", 3) != 0)
        return false;
    Utf8Out text(out, kMaxArtistLen);
    decodeLatin1(b + 33, 30, text);
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
