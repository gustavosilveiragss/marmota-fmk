#include "Menu.h"
#include "../Gfx.h"

namespace mrm {
namespace ui {

namespace {

void signal(Panel& o, int16_t x, int16_t bottom, int8_t rssi) {
    const uint8_t strength = rssi > -60 ? 4 : rssi > -70 ? 3
                                          : rssi > -80   ? 2
                                                         : 1;
    for (uint8_t i = 0; i < 4; ++i) {
        const int16_t h = 2 + i * 2;
        if (i < strength)
            o.fillRect(x + i * 4, bottom - h, 3, h);
        else
            o.drawHorizontalLine(x + i * 4, bottom - 1, 3);
    }
}

void toggleSwitch(Panel& o, int16_t x, int16_t y, bool on, float move) {
    o.drawRect(x, y, 14, 7);
    o.fillRect(x + 2 + int16_t((on ? move : 1 - move) * 6), y + 2, 4, 3);
}

} // namespace

void menuScreen(Panel& o, const Cursor& cursor, uint32_t now, const Menu& m) {
    const uint8_t count = m.count;
    const Row* rows = m.rows;
    constexpr uint8_t kVisible = 3;
    hintBar(o, m.hints, m.hintCount);
    o.setFont(ArialMT_Plain_10);
    clipped(o, 4, 13, m.title, kW - 8);
    o.drawHorizontalLine(0, 26, kW);

    const float pos = lerp(cursor.prev, cursor.pos, eased(now, cursor.at, kMoveMs));
    const float top = count > kVisible ? gfx::clamp01((pos - 1) / (count - kVisible)) * (count - kVisible) : 0;

    for (uint8_t i = 0; i < count; ++i) {
        const int16_t y = kBodyY + 1 + int16_t((i - top) * kRow);
        if (y < kBodyY - 2 || y > kH - kRow + 2)
            continue;
        const Row& r = rows[i];
        const bool picked = fabsf(pos - i) < 0.5f;
        if (picked) {
            o.fillRect(0, y, kW, kRow - 1);
            o.setColor(BLACK);
        }
        int16_t right = kW - 4;
        if (r.rssi != 0) {
            signal(o, kW - 19, y + 10, r.rssi);
            right = kW - 24;
        } else if (r.toggle >= 0) {
            toggleSwitch(o, kW - 18, y + 2, r.toggle == 1, r.toggleMove);
            right = kW - 22;
        } else if (r.value) {
            o.setFont(ArialMT_Plain_10);
            o.setTextAlignment(TEXT_ALIGN_RIGHT);
            const int16_t room = kW / 2 - 4;
            char val[34];
            strlcpy(val, r.value, sizeof(val));
            while (strlen(val) > 1 && textWidth(o, val) > room) {
                size_t n = strlen(val) - 1;
                while (n > 1 && (val[n] & 0xC0) == 0x80)
                    --n;
                val[n] = '\0';
            }
            o.drawText(kW - 4, y, val);
            right = kW - 8 - textWidth(o, val);
        }
        if (r.check) {
            o.drawLine(4, y + 6, 6, y + 8);
            o.drawLine(6, y + 8, 10, y + 3);
        }
        o.setFont(ArialMT_Plain_10);
        clipped(o, r.check ? 14 : 4, y, r.label, right - (r.check ? 14 : 4));
        o.setColor(WHITE);
    }
}

} // namespace ui
} // namespace mrm
