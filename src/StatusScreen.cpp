#include "StatusScreen.h"

namespace mrm {

namespace {

constexpr int16_t kPad = 2;
constexpr int16_t kBarHeight = 12;
constexpr int16_t kBarTextY = 1;
constexpr int16_t kPitch = 12;    // body line spacing
constexpr uint8_t kPageSteps = 3; // draws per page when lines overflow the panel

constexpr int16_t kWifiW = 8;
constexpr int16_t kWifiH = 8;
constexpr int16_t kWifiY = 2;
constexpr uint8_t kWifiFrames = 4;

// 8x8 wifi glyph, arcs growing out of a base dot (XBM rows, LSB = leftmost pixel).
const uint8_t kWifi[kWifiFrames][kWifiH] = {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x18, 0x00},
    {0x00, 0x00, 0x3c, 0x42, 0x18, 0x00, 0x18, 0x00},
    {0x7e, 0x81, 0x3c, 0x42, 0x18, 0x00, 0x18, 0x00},
};

} // namespace

void StatusScreen::draw(const Config& config, uint8_t step) {
    SSD1306Wire& oled = display_.raw();
    const int16_t width = oled.getWidth();
    const int16_t height = oled.getHeight();

    oled.clear();
    oled.setFont(ArialMT_Plain_10);

    oled.setColor(WHITE);
    oled.fillRect(0, 0, width, kBarHeight);
    oled.setColor(BLACK);

    int16_t titleX = kPad;
    if (config.showWifi) {
        oled.drawXbm(kPad, kWifiY, kWifiW, kWifiH, kWifi[step % kWifiFrames]);
        titleX += kWifiW + kPad;
    }
    oled.setTextAlignment(TEXT_ALIGN_LEFT);
    oled.drawString(titleX, kBarTextY, config.title);
    if (config.showBattery) {
        oled.setTextAlignment(TEXT_ALIGN_RIGHT);
        oled.drawString(width - kPad, kBarTextY, String(config.battery) + "%");
    }

    oled.setColor(WHITE);
    oled.setTextAlignment(TEXT_ALIGN_CENTER);
    const int16_t top = kBarHeight + 1;
    const int16_t fit = (height - top) / kPitch;
    if (fit > 0 && config.lineCount > 0) {
        const uint8_t perPage = fit < config.lineCount ? uint8_t(fit) : config.lineCount;
        const uint8_t pages = (config.lineCount + perPage - 1) / perPage;
        const uint8_t page = pages > 1 ? (step / kPageSteps) % pages : 0;
        for (uint8_t i = 0; i < perPage; ++i) {
            const uint8_t index = page * perPage + i;
            if (index >= config.lineCount)
                break;
            oled.drawString(width / 2, top + i * kPitch, config.lines[index]);
        }
    }

    oled.display();
}

} // namespace mrm
