#pragma once

#include <Arduino.h>

namespace mrm {

// Botao unico reduzido a uma primitiva: quantos cliques houve numa rajada. Quem chama mapeia a
// contagem pra acoes (1 = next, 2 = menu, N = atalho), entao ha um caminho unico de debounce e
// contagem, em vez de tratar cada tipo de clique separado.
class Button {
public:
    struct Config {
        uint16_t debounceMs = 30;
        uint16_t gapMs = 360; // tempo de silencio que fecha uma rajada de cliques
        bool activeLow = true;
    };

    explicit Button(uint8_t pin)
        : pin_(pin) {}
    Button(uint8_t pin, const Config& config)
        : pin_(pin)
        , config_(config) {}

    void begin() {
        pinMode(pin_, config_.activeLow ? INPUT_PULLUP : INPUT);
    }

    // Descarta uma rajada em andamento pra cliques de um contexto nao vazarem pro proximo.
    void reset() {
        raw_ = false;
        stable_ = false;
        count_ = 0;
        edgeAt_ = lastChange_ = 0;
    }

    // Numero de cliques da rajada que acabou, reportado quando o botao ficou solto por gapMs.
    // Retorna 0 no resto do tempo.
    uint8_t clicks() {
        const uint32_t now = millis();
        const bool down = held();

        if (down != raw_) {
            raw_ = down;
            edgeAt_ = now;
        }
        if (now - edgeAt_ >= config_.debounceMs && down != stable_) {
            stable_ = down;
            lastChange_ = now;
            if (stable_ && count_ < 250)
                ++count_;
        }
        if (count_ > 0 && !stable_ && now - lastChange_ >= config_.gapMs) {
            const uint8_t n = count_;
            count_ = 0;
            return n;
        }
        return 0;
    }

private:
    bool held() const {
        const int level = digitalRead(pin_);
        return config_.activeLow ? level == LOW : level == HIGH;
    }

    uint8_t pin_;
    Config config_{};
    bool raw_ = false;
    bool stable_ = false;
    uint8_t count_ = 0;
    uint32_t edgeAt_ = 0;
    uint32_t lastChange_ = 0;
};

} // namespace mrm
