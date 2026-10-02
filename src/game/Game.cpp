#include "Game.h"

#include "../ui/Draw.h"

namespace mrm {
namespace game {

void hud(Panel& o, uint16_t score, uint8_t lives) {
    for (uint8_t i = 0; i < lives; ++i)
        o.fillCircle(4 + i * 8, 5, 2);
    char text[8];
    snprintf(text, sizeof(text), "%u", score);
    o.setFont(ArialMT_Plain_10);
    o.setTextAlignment(TEXT_ALIGN_RIGHT);
    o.drawText(ui::kW - 2, 0, text);
    o.drawHorizontalLine(0, kFieldTop - 1, ui::kW);
}

} // namespace game
} // namespace mrm
