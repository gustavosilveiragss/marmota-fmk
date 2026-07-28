#pragma once

#include "Ssd1306Display.h"

namespace mrm {

// Uma barra de topo (glifo wifi opcional, titulo e bateria) sobre linhas centralizadas. O numero
// de linhas visiveis segue a altura do painel, entao a mesma tela cabe em displays menores
// paginando as linhas ao longo de draws sucessivos.
class StatusScreen {
public:
    struct Config {
        const char* title = "";
        const char* const* lines = nullptr;
        uint8_t lineCount = 0;
        uint8_t battery = 0;
        bool showBattery = false;
        bool showWifi = false;
    };

    explicit StatusScreen(Ssd1306Display& display)
        : display_(display) {}

    // step avanca a animacao do wifi e a paginacao de linhas em paineis pequenos.
    void draw(const Config& config, uint8_t step = 0);

private:
    Ssd1306Display& display_;
};

} // namespace mrm
