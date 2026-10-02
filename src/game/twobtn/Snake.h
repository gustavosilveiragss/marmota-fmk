#pragma once

#include "../Game.h"

namespace mrm {
namespace game {
namespace twobtn {

// Snake do Nokia: A vira a cobra para a esquerda dela, B para a direita. Paredes dao a volta.
class Snake : public Game {
public:
    const GameText& text() const override;
    void reset(uint32_t seed) override;
    void tick(const GameInput& in) override;
    void draw(mrm::Panel& o, uint32_t now) const override;
    uint8_t demo(mrm::Panel& o, uint32_t now) const override;
    bool over() const override { return over_; }
    uint16_t score() const override { return score_; }

private:
    static constexpr uint8_t kCols = 32;
    static constexpr uint8_t kRows = 13;
    static constexpr uint16_t kCells = kCols * kRows;
    static constexpr uint16_t kNone = 0xffff;

    bool occupied(uint16_t cell) const { return bits_[cell >> 3] & (1u << (cell & 7)); }
    void mark(uint16_t cell, bool on);
    void push(uint16_t cell);
    void pop();
    uint16_t freeCell();
    void step();
    uint16_t headCell() const { return body_[(tail_ + length_ - 1) % kCells]; }
    uint8_t stepTicks() const { return max<int>(3, 7 - length_ / 8); }

    uint16_t body_[kCells];
    uint8_t bits_[(kCells + 7) / 8];
    uint16_t tail_ = 0;
    uint16_t length_ = 0;
    uint16_t food_ = kNone;
    uint16_t bonus_ = kNone;
    uint16_t bonusTicks_ = 0;
    uint16_t score_ = 0;
    uint16_t eaten_ = 0;
    uint8_t grow_ = 0;
    uint8_t dir_ = 0; // 0 direita, 1 baixo, 2 esquerda, 3 cima
    int8_t turn_ = 0; // -1 esquerda, +1 direita, pedido pendente para o proximo passo
    uint8_t sinceStep_ = 0;
    bool over_ = false;
    uint32_t rng_ = 1;
};

} // namespace twobtn
} // namespace game
} // namespace mrm
