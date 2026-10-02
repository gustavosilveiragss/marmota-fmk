#include "GameScreen.h"

#include "../Gfx.h"
#include "../ui/Menu.h"
#include "GameText.h"

namespace mrm {
namespace game {

using namespace ui;

namespace {

constexpr int16_t kKeyRowY = 50;
constexpr int16_t kDotsY = 62;

void keycap(Panel& o, int16_t x, int16_t y, const char* key, bool lit) {
    o.setFont(ArialMT_Plain_10);
    const int16_t w = max<int16_t>(11, textWidth(o, key) + 4);
    o.setColor(WHITE);
    if (lit)
        o.fillRect(x, y, w, 11);
    else
        o.drawRect(x, y, w, 11);
    o.setColor(lit ? BLACK : WHITE);
    o.setTextAlignment(TEXT_ALIGN_LEFT);
    o.drawText(x + (w - textWidth(o, key)) / 2, y - 1, key);
    o.setColor(WHITE);
}

// A e B no pe do tutorial, cada uma com o que faz neste jogo; acende junto com a cena.
void keyRow(Panel& o, const GameText& gt, uint8_t lit, Lang lang) {
    keycap(o, 2, kKeyRowY, "A", lit & Game::kLitA);
    o.setTextAlignment(TEXT_ALIGN_LEFT);
    clipped(o, 17, kKeyRowY - 1, tr(lang, gt.keyA), kW / 2 - 20);
    const int16_t bx = kW - 13;
    keycap(o, bx, kKeyRowY, "B", lit & Game::kLitB);
    o.setTextAlignment(TEXT_ALIGN_RIGHT);
    o.drawText(bx - 4, kKeyRowY - 1, tr(lang, gt.keyB));
}

// Uma bolinha por pagina no pe: a pagina atual vira uma barrinha.
void pageDots(Panel& o, uint8_t page) {
    for (uint8_t i = 0; i < kTutorialPages; ++i) {
        const int16_t x = kW / 2 + (2 * i - (kTutorialPages - 1)) * 4;
        if (i == page)
            o.fillRect(x - 3, kDotsY, 6, 2);
        else
            o.fillRect(x - 1, kDotsY, 2, 2);
    }
}

// Barra de instrucoes do tutorial. Pagina 1: B avanca e A+B sai. Pagina 2 so tem o que importa ali:
// A volta para a pagina 1 e B joga (ou, vindo da pausa, B confirma e volta a ela).
void tutorialHints(Panel& o, const GameView& v, Lang lang) {
    if (v.page + 1 < kTutorialPages) {
        const Hint items[] = {{"B", tr(lang, Str::HintNext)}, {"A+B", tr(lang, v.fromPause ? Str::HintBack : Str::HintExit)}};
        hintBar(o, items, 2);
    } else {
        const Hint items[] = {{"A", tr(lang, Str::HintBack)}, {"B", tr(lang, v.fromPause ? Str::HintOk : Str::HintPlay)}};
        hintBar(o, items, 2);
    }
}

void tutorial(Panel& o, const GameView& v, uint32_t now, Lang lang) {
    const GameText& gt = v.game->text();
    tutorialHints(o, v, lang);
    if (v.page == 0) {
        centered(o, ArialMT_Plain_10, kW / 2, 13, tr(lang, gt.goal));
        keyRow(o, gt, v.game->demo(o, now), lang);
    } else {
        for (uint8_t i = 0; i < 3; ++i)
            centered(o, ArialMT_Plain_10, kW / 2, 15 + i * 13, tr(lang, gt.rules[i]));
    }
    pageDots(o, v.page);
}

void countdown(Panel& o, const GameView& v, uint32_t now) {
    v.game->draw(o, now);
    const uint32_t elapsed = now - v.phaseAt;
    const uint8_t digit = 3 - min<uint32_t>(2, elapsed * 3 / kCountdownMs);
    const float within = progressOf(elapsed % (kCountdownMs / 3), kCountdownMs / 3);
    clear(o, kW / 2 - 16, 24, 32, 32);
    o.drawCircle(kW / 2, 40, int16_t(20 - 8 * gfx::easeOut(within)));
    char text[2] = {char('0' + digit), '\0'};
    centered(o, ArialMT_Plain_24, kW / 2, 26, text);
}

void paused(Panel& o, const GameView& v, uint32_t now, Lang lang) {
    const Hint hintItems[] = {{"A", tr(lang, Str::HintDown)}, {"B", tr(lang, Str::HintOk)}, {"A+B", tr(lang, Str::HintBack)}};
    const Row rows[] = {Row{tr(lang, Str::Resume)}, Row{tr(lang, Str::HowTo)}, Row{tr(lang, Str::Exit)}};
    menuScreen(o, v.cursor, now, {hintItems, 3, tr(lang, Str::Paused), rows, 3});
}

void over(Panel& o, const GameView& v, uint32_t now, Lang lang) {
    const Hint hintItems[] = {{"A", tr(lang, Str::HintDown)}, {"B", tr(lang, Str::HintOk)}, {"A+B", tr(lang, Str::HintBack)}};
    const Row rows[] = {Row{tr(lang, Str::PlayAgain)}, Row{tr(lang, Str::Exit)}};
    char title[40];
    if (v.record)
        snprintf(title, sizeof(title), "%u  %s", v.score, tr(lang, Str::NewRecord));
    else
        snprintf(title, sizeof(title), "%u  (%s %u)", v.score, tr(lang, Str::Best), v.best);
    menuScreen(o, v.cursor, now, {hintItems, 3, title, rows, 2});
}

} // namespace

void screen(Panel& o, const GameView& v, uint32_t now, Lang lang) {
    switch (v.phase) {
    case GamePhase::Tutorial:
        tutorial(o, v, now, lang);
        break;
    case GamePhase::Countdown:
        countdown(o, v, now);
        break;
    case GamePhase::Playing:
        v.game->draw(o, now);
        break;
    case GamePhase::Paused:
        paused(o, v, now, lang);
        break;
    case GamePhase::Over:
        over(o, v, now, lang);
        break;
    }
}

} // namespace game
} // namespace mrm
