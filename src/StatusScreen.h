#pragma once

#include "Ssd1306Display.h"

namespace mrm {

// A top bar (optional wifi glyph, title and battery) over centered body lines.
// The number of visible lines follows the panel height, so the same screen also
// fits smaller displays by paging through the lines across successive draws.
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

    // step advances the wifi animation and the line paging on small panels.
    void draw(const Config& config, uint8_t step = 0);

private:
    Ssd1306Display& display_;
};

} // namespace mrm
