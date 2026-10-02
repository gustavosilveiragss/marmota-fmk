#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

#if __has_include(<WebServer.h>)
#include <WebServer.h>
#define MRM_HAS_WEBSERVER 1
#endif

namespace mrm {

// Escapa um caractere de string JSON em out (precisa de 6 bytes); devolve quantos escreveu.
// Bytes >= 0x80 (UTF-8) passam intactos.
inline size_t jsonEscapeChar(unsigned char c, char* out) {
    switch (c) {
    case '"':
    case '\\':
        out[0] = '\\';
        out[1] = static_cast<char>(c);
        return 2;
    case '\n':
        out[0] = '\\';
        out[1] = 'n';
        return 2;
    case '\r':
        out[0] = '\\';
        out[1] = 'r';
        return 2;
    case '\t':
        out[0] = '\\';
        out[1] = 't';
        return 2;
    default:
        break;
    }
    if (c >= 0x20) {
        out[0] = static_cast<char>(c);
        return 1;
    }
    static const char hex[] = "0123456789ABCDEF";
    out[0] = '\\';
    out[1] = 'u';
    out[2] = '0';
    out[3] = '0';
    out[4] = hex[c >> 4];
    out[5] = hex[c & 15];
    return 6;
}

// Escapa `in` ate acabar ou faltar espaco para o proximo caractere. `in` avanca ate onde foi
// consumido (== '\0' quando terminou). Nao grava terminador; devolve bytes escritos em out.
inline size_t jsonEscapeSome(const char*& in, char* out, size_t cap) {
    size_t n = 0;
    while (*in) {
        char tmp[6];
        const size_t len = jsonEscapeChar(static_cast<unsigned char>(*in), tmp);
        if (n + len > cap)
            break;
        for (size_t i = 0; i < len; ++i)
            out[n++] = tmp[i];
        ++in;
    }
    return n;
}

// Versao completa: escapa `in` inteira para out (cap inclui o '\0'), truncando em fronteira de
// caractere se nao couber. Devolve o comprimento escrito, sem o terminador.
inline size_t jsonEscape(const char* in, char* out, size_t cap) {
    if (cap == 0)
        return 0;
    const size_t n = jsonEscapeSome(in, out, cap - 1);
    out[n] = '\0';
    return n;
}

#ifdef MRM_HAS_WEBSERVER
// Escreve JSON em chunks de ate 256 B. O chamador faz setContentLength(CONTENT_LENGTH_UNKNOWN)
// e send() antes; end() fecha o chunked.
class JsonOut {
public:
    explicit JsonOut(WebServer& server) : server_(server) {}

    void raw(const char* s) {
        while (*s) {
            const size_t room = sizeof(buf_) - len_;
            const size_t take = strnlen(s, room);
            memcpy(buf_ + len_, s, take);
            len_ += take;
            s += take;
            if (len_ == sizeof(buf_))
                flush();
        }
    }

    // Escreve s entre aspas, escapada.
    void str(const char* s) {
        put('"');
        while (*s) {
            len_ += jsonEscapeSome(s, buf_ + len_, sizeof(buf_) - len_);
            if (*s)
                flush();
        }
        put('"');
    }

    void num(uint64_t v) {
        char tmp[24];
        snprintf(tmp, sizeof(tmp), "%llu", static_cast<unsigned long long>(v));
        raw(tmp);
    }

    void end() {
        flush();
        server_.sendContent("", 0);
    }

private:
    WebServer& server_;
    char buf_[256];
    size_t len_ = 0;

    void put(char c) {
        if (len_ == sizeof(buf_))
            flush();
        buf_[len_++] = c;
    }

    void flush() {
        if (len_ == 0)
            return;
        server_.sendContent(buf_, len_);
        len_ = 0;
    }
};

#endif // MRM_HAS_WEBSERVER

} // namespace mrm
