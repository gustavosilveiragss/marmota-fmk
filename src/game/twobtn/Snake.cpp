#include "Snake.h"

#include "Text.h"

namespace mrm {
namespace game {
namespace twobtn {

namespace {

constexpr int16_t kCell = 4;
constexpr uint8_t kStartCol = 14;
constexpr uint8_t kStartRow = 6;
constexpr uint8_t kStartLength = 3;
constexpr uint8_t kBonusEvery = 5;
constexpr uint16_t kBonusTicks = 70;
constexpr uint8_t kBonusPoints = 3;
constexpr GameText kText = {
    Str::GameSnake, Str::GoalSnake, Str::KeyLeft, Str::KeyRight, {Str::SnakeR1, Str::SnakeR2, Str::SnakeR3}};

} // namespace

const GameText& Snake::text() const {
    return kText;
}

void Snake::mark(uint16_t cell, bool on) {
    if (on)
        bits_[cell >> 3] |= 1u << (cell & 7);
    else
        bits_[cell >> 3] &= ~(1u << (cell & 7));
}

void Snake::push(uint16_t cell) {
    body_[(tail_ + length_) % kCells] = cell;
    ++length_;
    mark(cell, true);
}

void Snake::pop() {
    mark(body_[tail_], false);
    tail_ = (tail_ + 1) % kCells;
    --length_;
}

// Sorteia uma célula livre. Se o sorteio não achar, varre. kNone só com o tabuleiro cheio.
uint16_t Snake::freeCell() {
    for (uint8_t i = 0; i < 24; ++i) {
        const uint16_t cell = nextRandom(rng_) % kCells;
        if (!occupied(cell) && cell != food_ && cell != bonus_)
            return cell;
    }

    for (uint16_t cell = 0; cell < kCells; ++cell) {
        if (!occupied(cell) && cell != food_ && cell != bonus_)
            return cell;
    }

    return kNone;
}

void Snake::reset(uint32_t seed) {
    memset(bits_, 0, sizeof(bits_));
    rng_ = seed | 1;
    tail_ = length_ = 0;

    for (uint8_t i = 0; i < kStartLength; ++i)
        push(kStartRow * kCols + kStartCol + i);

    food_ = bonus_ = kNone;
    food_ = freeCell();

    bonusTicks_ = 0;

    score_ = 0;
    eaten_ = 0;
    grow_ = sinceStep_ = 0;
    dir_ = 0;
    turn_ = 0;
    over_ = false;
}

void Snake::tick(const GameInput& in) {
    if (in.aPress)
        turn_ = -1;
    else if (in.bPress)
        turn_ = 1;

    if (bonusTicks_ && --bonusTicks_ == 0)
        bonus_ = kNone;

    if (++sinceStep_ >= stepTicks()) {
        sinceStep_ = 0;
        step();
    }
}

void Snake::step() {
    dir_ = (dir_ + 4 + turn_) % 4;
    turn_ = 0;

    const uint16_t head = headCell();

    int col = head % kCols;
    int row = head / kCols;
    static constexpr int8_t kDx[] = {1, 0, -1, 0};
    static constexpr int8_t kDy[] = {0, 1, 0, -1};

    col = (col + kDx[dir_] + kCols) % kCols;
    row = (row + kDy[dir_] + kRows) % kRows;
    const uint16_t next = row * kCols + col;

    if (grow_)
        --grow_;
    else
        pop(); // o rabo sai antes do teste: seguir o próprio rabo não e batida

    if (occupied(next)) {
        over_ = true;
        return;
    }

    push(next);

    if (next == food_) {
        ++score_;
        ++grow_;
        food_ = freeCell();

        if (++eaten_ % kBonusEvery == 0) {
            bonus_ = freeCell();
            bonusTicks_ = kBonusTicks;
        }
    } else if (next == bonus_) {
        score_ += kBonusPoints;
        ++grow_;

        bonus_ = kNone;
        bonusTicks_ = 0;
    }

    if (food_ == kNone)
        over_ = true; // tabuleiro cheio: zerou o jogo
}

void Snake::draw(mrm::Panel& o, uint32_t now) const {
    hud(o, score_, 0);

    for (uint16_t i = 0; i < length_; ++i) {
        const uint16_t cell = body_[(tail_ + i) % kCells];
        const int16_t x = (cell % kCols) * kCell;
        const int16_t y = kFieldTop + (cell / kCols) * kCell;
        o.fillRect(x, y, i == length_ - 1 ? 4 : 3, i == length_ - 1 ? 4 : 3);
    }

    if (food_ != kNone)
        o.drawRect((food_ % kCols) * kCell, kFieldTop + (food_ / kCols) * kCell, 3, 3);

    if (bonus_ != kNone && (now / 140) % 2) // a bonus pisca: some logo
        o.fillRect((bonus_ % kCols) * kCell - 1, kFieldTop + (bonus_ / kCols) * kCell - 1, 5, 5);
}

// Escadinha: três para a direita, dois para baixo, repetindo. B acende na curva para baixo, A na volta.
uint8_t Snake::demo(mrm::Panel& o, uint32_t now) const {
    constexpr uint8_t kTail = 9;
    constexpr uint8_t kLoop = 50;
    const uint8_t step = (now / 140) % kLoop;
    uint8_t rights = 0;
    uint8_t downs = 0;
    for (uint8_t k = 0; k <= step; ++k) {
        if (k > 0)
            (k % 5 >= 3 ? downs : rights)++;

        if (step - k < kTail)
            o.fillRect(4 + (rights % 28) * kCell, 28 + (downs % 5) * kCell, 3, 3);
    }

    const uint8_t phase = step % 5;
    return phase == 3 ? kLitB : (phase == 0 && step > 0 ? kLitA : 0);
}

} // namespace twobtn
} // namespace game
} // namespace mrm
