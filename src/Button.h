#pragma once

#include <Arduino.h>

namespace mrm {

// Botão único reduzido a uma primitiva: quantos cliques houve numa rajada. Quem chama mapeia a
// contagem pra ações (1 = next, 2 = menu, N = atalho), então há um caminho único de debounce e
// contagem, em vez de tratar cada tipo de clique separado.
class Button {
public:
    struct Config {
        uint16_t debounceMs = 30;
        uint16_t gapMs = 360; // tempo de silêncio que fecha uma rajada de cliques
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

    // Descarta uma rajada em andamento pra cliques de um contexto não vazarem pro próximo.
    void reset() {
        level_ = false;
        count_ = 0;
        lastChange_ = 0;
    }

    // Número de cliques da rajada que acabou, reportado quando o botão ficou solto por gapMs.
    // Retorna 0 no resto do tempo. A contagem não depende da cadência do poll: o wake por borda do
    // light sleep acorda a CPU em cada aperto e soltura, então uma passada única por borda basta.
    uint8_t clicks() {
        const uint32_t now = millis();
        const bool down = held();

        // Fecha a rajada antes de abrir a próxima: solto por gapMs reporta já aqui, mesmo com um
        // aperto novo chegando no mesmo poll (ele entra na passada seguinte, com level_ ainda
        // solto), então dois cliques separados não colam quando o poll e esparso.
        if (count_ > 0 && !level_ && now - lastChange_ >= config_.gapMs) {
            const uint8_t n = count_;
            count_ = 0;
            return n;
        }

        // Lockout de debounce: uma troca de nível conta na primeira amostra dela desde que passem
        // debounceMs desde a última, então bounce e filtrado e nenhuma borda precisa ser reamostrada.
        if (down != level_ && now - lastChange_ >= config_.debounceMs) {
            level_ = down;
            lastChange_ = now;
            if (down && count_ < 250)
                ++count_;
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
    bool level_ = false;      // nível debounced corrente, true quando pressionado
    uint8_t count_ = 0;
    uint32_t lastChange_ = 0; // millis da última troca de nível aceita
};

} // namespace mrm
