#pragma once

#include <stddef.h>
#include <stdint.h>

namespace mrm {

// 48 kHz -> 44,1 kHz, PCM estereo intercalado, interpolacao cubica (Catmull-Rom) com a razao exata 160/147.
// Um MP3 a 48 kHz ja vem filtrado abaixo de ~20 kHz, menos que o Nyquist de 22,05 kHz, entao reduzir a taxa
// por interpolacao nao gera aliasing audivel. Nao usa heap: o estado sao 3 quadros de historico.
class Resample48to44 {
public:
    static constexpr uint32_t kInRate = 48000;

    void reset() { *this = Resample48to44(); }

    // Consome `frames` quadros de `in` e entrega os convertidos em blocos de ate kChunk quadros: sink(out, quadros).
    template <typename Sink>
    void feed(const int16_t* in, size_t frames, Sink sink) {
        int16_t out[kChunk * 2];
        size_t n = 0;
        const int32_t end = static_cast<int32_t>(frames);
        while (pos_ + 2 < end) { // o ponto precisa de 2 quadros a frente
            const float t = rem_ * (1.0f / kStepOut);
            for (int c = 0; c < 2; ++c) {
                const float p0 = at(in, pos_ - 1, c), p1 = at(in, pos_, c), p2 = at(in, pos_ + 1, c), p3 = at(in, pos_ + 2, c);
                const float v = p1 + 0.5f * t * (p2 - p0 + t * (2 * p0 - 5 * p1 + 4 * p2 - p3 + t * (3 * (p1 - p2) + p3 - p0)));
                out[2 * n + c] = static_cast<int16_t>(v > 32767.0f ? 32767.0f : v < -32768.0f ? -32768.0f : v);
            }
            if (++n == kChunk) {
                sink(out, n);
                n = 0;
            }
            rem_ += kStepIn;
            pos_ += rem_ / kStepOut;
            rem_ %= kStepOut;
        }
        if (n)
            sink(out, n);
        keepHistory(in, end);
        pos_ -= end;
    }

private:
    static constexpr size_t kChunk = 64;
    static constexpr int kStepIn = 160, kStepOut = 147; // 48000 / 44100 reduzido

    // Quadro k do bloco; os negativos sao os ultimos do bloco anterior.
    float at(const int16_t* in, int32_t k, int c) const { return k < 0 ? hist_[k + 3][c] : in[2 * k + c]; }

    void keepHistory(const int16_t* in, int32_t end) {
        int16_t next[3][2];
        for (int i = 0; i < 3; ++i)
            for (int c = 0; c < 2; ++c)
                next[i][c] = static_cast<int16_t>(at(in, end - 3 + i, c));
        for (int i = 0; i < 3; ++i)
            for (int c = 0; c < 2; ++c)
                hist_[i][c] = next[i][c];
    }

    int16_t hist_[3][2] = {};
    int32_t pos_ = 0; // quadro de entrada do proximo ponto, relativo ao inicio do bloco (>= -2)
    int rem_ = 0;     // fracao desse quadro, em 1/147
};

} // namespace mrm
