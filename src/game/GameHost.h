#pragma once

#include "../ui/Cursor.h"
#include "Game.h"
#include "GameStore.h"
#include "GameView.h"

namespace mrm {
namespace game {

// O que o jogador pediu nas telas de tutorial, pausa e fim: descer a selecao, confirmar ou voltar.
enum class Nav : uint8_t { None,
                           Down,
                           Confirm,
                           Back };

// Conduz uma partida: tutorial (so na primeira vez de cada jogo), contagem, jogo, pausa e fim.
// Os jogos so simulam e desenham; menu, pausa e recorde ficam aqui. Dentro do jogo as teclas sao
// lidas cruas (segurado/solto); os gestos de Nav so valem nos menus.
class GameHost {
public:
    // `games` fica com o chamador e deve viver mais que o host.
    GameHost(GameStore& store, Game* const* games, uint8_t count)
        : store_(store)
        , games_(games)
        , count_(count) {}

    void open(uint8_t index);
    void handle(Nav nav);
    void update(uint32_t now, bool a, bool b);
    void fill(GameView& view) const;

    bool exited() const { return exited_; }
    bool running() const { return phase_ == GamePhase::Countdown || phase_ == GamePhase::Playing; }
    uint8_t index() const { return index_; }

private:
    static constexpr uint32_t kPauseHoldMs = 700;   // as duas teclas juntas por este tempo pausam
    static constexpr uint32_t kIdlePauseMs = 20000; // sem tocar nas teclas: pausa para poupar bateria
    static constexpr uint8_t kPauseRows = 3;
    static constexpr uint8_t kOverRows = 2;

    void setPhase(GamePhase phase);
    void start();
    void pause();
    void resume();
    void finish();
    void runTicks(uint32_t now, bool a, bool b);
    void pick();
    void leaveTutorial();

    GameStore& store_;
    Game* const* games_;
    uint8_t count_;
    Game* game_ = nullptr;
    uint8_t index_ = 0;
    GamePhase phase_ = GamePhase::Tutorial;
    ui::Cursor cursor_;
    uint32_t phaseAt_ = 0;
    uint32_t lastTick_ = 0;
    uint32_t bothAt_ = 0;
    uint32_t inputAt_ = 0;
    bool prevA_ = false;
    bool prevB_ = false;
    bool fromPause_ = false;
    bool armed_ = false; // as duas teclas ja foram soltas desde que a fase comecou
    uint8_t page_ = 0;
    bool record_ = false;
    bool exited_ = false;
};

} // namespace game
} // namespace mrm
