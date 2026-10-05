#pragma once

#include <stdint.h>
#include <string.h>

namespace mrm {

// Anel de linhas de texto de largura fixa. Puro: quem usa cuida da memória, do tempo e da exclusão mútua.
template <uint8_t Lines, uint8_t Width>
struct LogRing {
    static constexpr uint32_t kMagic = 0x4C4F4721;

    uint32_t magic;
    uint8_t head; // próxima posição a escrever
    uint8_t count;
    char lines[Lines][Width];

    void clear() {
        memset(this, 0, sizeof(*this));
        magic = kMagic;
    }

    // Aceita o conteúdo que sobrou de um reset. Devolve false e zera quando a memória não é um anel nosso.
    bool recover() {
        if (magic != kMagic || head >= Lines || count > Lines) {
            clear();
            return false;
        }

        for (uint8_t i = 0; i < Lines; ++i) {
            lines[i][Width - 1] = '\0';
            printable(lines[i]);
        }

        return true;
    }

    // Memória RTC com lixo parcial (queda de energia) vira "?" em vez de lixo na tela.
    static void printable(char* line) {
        for (char* p = line; *p; ++p) {
            if (*p < ' ' || *p > '~')
                *p = '?';
        }
    }

    // Trunca no tamanho da linha e descarta quebras de linha. Texto vazio não entra.
    void push(const char* text) {
        char* line = lines[head];
        uint8_t used = 0;

        for (const char* p = text; *p && used < Width - 1; ++p) {
            if (*p == '\r' || *p == '\n')
                continue;

            line[used++] = *p;
        }

        line[used] = '\0';
        if (used == 0)
            return;

        head = (head + 1) % Lines;
        if (count < Lines)
            ++count;
    }

    // Posição 0 é a linha mais antiga.
    const char* at(uint8_t index) const {
        const uint8_t first = (head + Lines - count) % Lines;
        return lines[(first + index) % Lines];
    }
};

} // namespace mrm
