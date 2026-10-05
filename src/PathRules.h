#pragma once

#include <cstddef>
#include <cstring>
#include <strings.h>

namespace mrm {

constexpr size_t kMaxPlaylistLen = 48;
constexpr size_t kMaxTrackLen = 48; // com ".mp3"

// Nomes viram caminhos no cartão e vêm da rede: nada de separador, curinga, controle nem ponto
// ou espaço nas pontas (o ".." fica barrado por isso).
inline bool validName(const char* s, size_t maxLen) {
    const size_t n = s ? strlen(s) : 0;
    if (n == 0 || n > maxLen)
        return false;

    if (s[0] == ' ' || s[0] == '.' || s[n - 1] == ' ' || s[n - 1] == '.')
        return false;

    for (size_t i = 0; i < n; ++i) {
        const unsigned char c = static_cast<unsigned char>(s[i]);
        if (c < 0x20 || c == 0x7f || strchr("/\\:*?\"<>|", c))
            return false;
    }

    return true;
}

inline bool validPlaylistName(const char* s) {
    return validName(s, kMaxPlaylistLen);
}

inline bool validTrackName(const char* s) {
    const size_t n = s ? strlen(s) : 0;
    return n > 4 && validName(s, kMaxTrackLen) && strcasecmp(s + n - 4, ".mp3") == 0;
}

} // namespace mrm
