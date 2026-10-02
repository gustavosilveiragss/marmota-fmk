#include "GameHost.h"

namespace mrm {
namespace game {

namespace {
uint32_t stamp() {
    return max<uint32_t>(millis(), 1);
}
} // namespace

void GameHost::setPhase(GamePhase phase) {
    phase_ = phase;
    phaseAt_ = stamp();
    cursor_.reset();
    armed_ = false; // soltar as teclas da fase anterior nao pode agir na nova (pausa, fim de partida)
    if (phase == GamePhase::Tutorial)
        page_ = 0;
}

void GameHost::open(uint8_t index) {
    index_ = min<uint8_t>(index, count_ - 1);
    game_ = games_[index_];
    exited_ = false;
    fromPause_ = false;
    if (store_.tutorialSeen(index_))
        start();
    else
        setPhase(GamePhase::Tutorial);
}

void GameHost::start() {
    store_.setTutorialSeen(index_);
    game_->reset(micros());
    record_ = false;
    prevA_ = prevB_ = false;
    setPhase(GamePhase::Countdown);
}

void GameHost::resume() {
    prevA_ = prevB_ = false;
    setPhase(GamePhase::Countdown);
}

void GameHost::pause() {
    setPhase(GamePhase::Paused);
}

void GameHost::finish() {
    record_ = store_.setBest(index_, game_->score());
    setPhase(GamePhase::Over);
}

void GameHost::leaveTutorial() {
    if (fromPause_) {
        fromPause_ = false;
        pause();
        cursor_.reset(1); // volta em Como jogar, de onde veio
    } else {
        start();
    }
}

void GameHost::pick() {
    if (phase_ == GamePhase::Paused) {
        if (cursor_.pos == 0) {
            resume();
        } else if (cursor_.pos == 1) {
            fromPause_ = true;
            setPhase(GamePhase::Tutorial);
        } else {
            exited_ = true;
        }
    } else if (cursor_.pos == 0) {
        start(); // jogar de novo, sem tutorial
    } else {
        exited_ = true;
    }
}

void GameHost::handle(Nav nav) {
    if (!armed_)
        return;
    const bool down = nav == Nav::Down;
    const bool confirm = nav == Nav::Confirm;
    const bool back = nav == Nav::Back;

    switch (phase_) {
    case GamePhase::Tutorial:
        if (down && page_ > 0)
            --page_;
        else if (confirm && page_ + 1 < kTutorialPages)
            ++page_;
        else if (confirm)
            leaveTutorial();
        else if (back && fromPause_)
            leaveTutorial();
        else if (back)
            exited_ = true;
        break;
    case GamePhase::Paused:
        if (down)
            cursor_.move(kPauseRows);
        else if (confirm)
            pick();
        else if (back)
            resume();
        break;
    case GamePhase::Over:
        if (down)
            cursor_.move(kOverRows);
        else if (confirm)
            pick();
        else if (back)
            exited_ = true;
        break;
    default:
        break; // contagem e jogo nao usam acoes
    }
}

void GameHost::runTicks(uint32_t now, bool a, bool b) {
    uint8_t budget = 3; // depois de um engasgo nao tenta recuperar tudo de uma vez
    while (now - lastTick_ >= Game::kTickMs && budget--) {
        lastTick_ += Game::kTickMs;
        game_->tick({a, b, a && !prevA_, b && !prevB_});
        prevA_ = a;
        prevB_ = b;
        if (game_->over()) {
            finish();
            return;
        }
    }
    if (now - lastTick_ >= Game::kTickMs)
        lastTick_ = now;
}

void GameHost::update(uint32_t now, bool a, bool b) {
    if (!a && !b)
        armed_ = true;
    if (phase_ == GamePhase::Countdown && now - phaseAt_ >= kCountdownMs) {
        setPhase(GamePhase::Playing);
        prevA_ = a; // tecla ja apertada na contagem nao conta como toque novo
        prevB_ = b;
        lastTick_ = now;
        bothAt_ = 0;
        inputAt_ = now;
    }
    if (phase_ != GamePhase::Playing)
        return;

    if (a || b)
        inputAt_ = now;
    if (a && b) {
        bothAt_ = bothAt_ ? bothAt_ : now;
    } else {
        bothAt_ = 0;
    }
    if ((bothAt_ && now - bothAt_ >= kPauseHoldMs) || now - inputAt_ >= kIdlePauseMs) {
        pause();
        return;
    }
    runTicks(now, a, b);
}

void GameHost::fill(GameView& v) const {
    v.game = game_;
    v.phase = phase_;
    v.index = index_;
    v.score = game_ ? game_->score() : 0;
    v.best = store_.best(index_);
    v.record = record_;
    v.fromPause = fromPause_;
    v.page = page_;
    v.phaseAt = phaseAt_;
    v.cursor = cursor_;
}

} // namespace game
} // namespace mrm
