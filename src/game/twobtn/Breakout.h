#pragma once

#include "../Game.h"

namespace mrm {
namespace game {
namespace twobtn {

// Breakout (Bounce): A move a raquete para a esquerda, B para a direita. A bola sai sozinha.
class Breakout : public Game {
public:
    const GameText& text() const override;
    void reset(uint32_t seed) override;
    void tick(const GameInput& in) override;
    void draw(mrm::Panel& o, uint32_t now) const override;
    uint8_t demo(mrm::Panel& o, uint32_t now) const override;
    bool over() const override { return lives_ == 0; }
    uint16_t score() const override { return score_; }

private:
    static constexpr uint8_t kBrickRows = 4;
    static constexpr uint8_t kBrickCols = 10;

    void buildBricks();
    void serve();
    void launch();
    void moveBall();
    void bounceOnPaddle();
    bool hitBrick(int16_t px, int16_t py);

    uint16_t bricks_[kBrickRows]; // bit c da linha r: tijolo presente
    int32_t bx_ = 0;              // bola em 1/16 de pixel
    int32_t by_ = 0;
    int32_t vx_ = 0;
    int32_t vy_ = 0;
    int16_t paddleX_ = 0;
    uint16_t score_ = 0;
    uint8_t lives_ = 0;
    uint8_t level_ = 0;
    uint8_t stuck_ = 0; // ticks restantes com a bola na raquete
    uint8_t left_ = 0;
};

} // namespace twobtn
} // namespace game
} // namespace mrm
