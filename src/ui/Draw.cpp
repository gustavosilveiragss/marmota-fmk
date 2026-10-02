#include "Draw.h"
#include "../Gfx.h"

namespace mrm {
namespace ui {

constexpr size_t kClipLen = 48; // maior texto que clipped encurta, sem contar o fim

float progressOf(uint32_t elapsed, uint32_t duration) {
    return gfx::clamp01(float(elapsed) / duration);
}

float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

// Progresso de uma transicao que comecou em `at`; 1 se nunca aconteceu.
float eased(uint32_t now, uint32_t at, uint32_t duration) {
    return at == 0 ? 1.0f : gfx::easeOut(progressOf(now - at, duration));
}

int16_t textWidth(Panel& o, const char* text) {
    return o.getStringWidth(text, strlen(text), true);
}

void centered(Panel& o, const uint8_t* font, int16_t cx, int16_t y, const char* text) {
    o.setFont(font);
    o.setTextAlignment(TEXT_ALIGN_CENTER);
    o.drawText(cx, y, text);
}

// Encurta com "." no fim ate caber, sem alocar.
void clipped(Panel& o, int16_t x, int16_t y, const char* text, int16_t maxWidth) {
    char buf[kClipLen + 1];
    strlcpy(buf, text, sizeof(buf));
    size_t len = strlen(buf);
    while (len > 1 && textWidth(o, buf) > maxWidth) {
        buf[--len] = '\0';
        while (len > 1 && (buf[len - 1] & 0xC0) == 0x80) // nao corta no meio de um caractere UTF-8
            buf[--len] = '\0';
        buf[len - 1] = '.';
    }
    o.setTextAlignment(TEXT_ALIGN_LEFT);
    o.drawText(x, y, buf);
}

void clear(Panel& o, int16_t x, int16_t y, int16_t w, int16_t h) {
    o.setColor(BLACK);
    o.fillRect(x, y, w, h);
    o.setColor(WHITE);
}

void hintBar(Panel& o, const Hint* items, uint8_t count) {
    constexpr int16_t kKeyPad = 1; // medido em scripts/layout.py: com mais folga a barra estoura em pt-BR
    constexpr int16_t kGap = 4;
    o.setColor(WHITE);
    o.fillRect(0, 0, kW, kBar);
    o.setFont(ArialMT_Plain_10);
    int16_t total = (count - 1) * kGap;
    for (uint8_t i = 0; i < count; ++i)
        total += textWidth(o, items[i].key) + 2 * kKeyPad + 3 + textWidth(o, items[i].label);

    int16_t x = max<int16_t>(2, (kW - total) / 2);
    for (uint8_t i = 0; i < count; ++i) {
        const int16_t keyW = textWidth(o, items[i].key) + 2 * kKeyPad;
        o.setColor(BLACK);
        o.fillRect(x, 1, keyW, 10);
        o.setColor(WHITE);
        o.setTextAlignment(TEXT_ALIGN_LEFT);
        o.drawText(x + kKeyPad, 0, items[i].key);
        o.setColor(BLACK);
        o.drawText(x + keyW + 3, 0, items[i].label);
        x += keyW + 3 + textWidth(o, items[i].label) + kGap;
    }
    o.setColor(WHITE);
}

} // namespace ui
} // namespace mrm
