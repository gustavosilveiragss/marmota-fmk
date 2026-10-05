#include "Breakout.h"

#include "Text.h"

namespace mrm {
namespace game {
namespace twobtn {

namespace {

constexpr int16_t kPaddleW = 22;
constexpr int16_t kPaddleY = 60;
constexpr int16_t kPaddleSpeed = 3;
constexpr int16_t kBrickTop = 15;
constexpr int16_t kBrickW = 11;
constexpr int16_t kBrickH = 5;
constexpr int16_t kBrickPitchX = 12;
constexpr int16_t kBrickPitchY = 6;
constexpr int16_t kBrickLeft = 4;
constexpr int32_t kFixed = 16;
constexpr int32_t kBaseSpeed = 22; // 1/16 px por passo
constexpr uint8_t kServeTicks = 36;
constexpr GameText kText = {
    Str::GameBreakout, Str::GoalBreakout, Str::KeyLeft, Str::KeyRight,
    {Str::BreakoutR1, Str::BreakoutR2, Str::BreakoutR3}};

} // namespace

const GameText& Breakout::text() const {
    return kText;
}

void Breakout::buildBricks() {
    for (uint16_t& row : bricks_)
        row = (1u << kBrickCols) - 1;

    left_ = kBrickRows * kBrickCols;
}

void Breakout::serve() {
    stuck_ = kServeTicks;
    vx_ = vy_ = 0;
    bx_ = (paddleX_ + kPaddleW / 2) * kFixed;
    by_ = (kPaddleY - 3) * kFixed;
}

void Breakout::reset(uint32_t seed) {
    (void)seed;
    paddleX_ = (128 - kPaddleW) / 2;
    score_ = 0;
    lives_ = 3;
    level_ = 0;
    buildBricks();
    serve();
}

void Breakout::launch() {
    const int32_t speed = kBaseSpeed + level_ * 3;
    vx_ = speed / 3;
    vy_ = -(speed - speed / 6);
}

void Breakout::tick(const GameInput& in) {
    if (in.a)
        paddleX_ -= kPaddleSpeed;

    if (in.b)
        paddleX_ += kPaddleSpeed;

    paddleX_ = constrain(paddleX_, 0, 128 - kPaddleW);

    if (stuck_) {
        bx_ = (paddleX_ + kPaddleW / 2) * kFixed;

        if (--stuck_ == 0)
            launch();

        return;
    }

    moveBall();
}

// Tijolo sob o ponto (px, py), se houver: remove, pontua e devolve true.
bool Breakout::hitBrick(int16_t px, int16_t py) {
    const int16_t row = (py - kBrickTop) / kBrickPitchY;
    const int16_t col = (px - kBrickLeft) / kBrickPitchX;
    if (py < kBrickTop || px < kBrickLeft || row >= kBrickRows || col >= kBrickCols || !(bricks_[row] & (1u << col)))
        return false;

    bricks_[row] &= ~(1u << col);
    score_ += (kBrickRows - row) * 10;
    --left_;
    return true;
}

void Breakout::bounceOnPaddle() {
    const int32_t speed = kBaseSpeed + level_ * 3;
    const float offset = constrain((bx_ / kFixed - paddleX_ - kPaddleW / 2.0f) / (kPaddleW / 2.0f), -1.0f, 1.0f);
    vx_ = static_cast<int32_t>(speed * offset * 0.85f);
    vy_ = -static_cast<int32_t>(sqrtf(static_cast<float>(speed * speed - vx_ * vx_)));
    by_ = (kPaddleY - 1) * kFixed;
}

void Breakout::moveBall() {
    bx_ += vx_;

    if (bx_ < 0 || bx_ > 126 * kFixed) {
        vx_ = -vx_;
        bx_ = constrain(bx_, 0, 126 * kFixed);
    }

    if (hitBrick(bx_ / kFixed, by_ / kFixed)) {
        vx_ = -vx_;
        bx_ += vx_ * 2;
    }

    by_ += vy_;

    if (by_ < kFieldTop * kFixed) {
        vy_ = -vy_;
        by_ = kFieldTop * kFixed;
    }

    if (hitBrick(bx_ / kFixed, by_ / kFixed))
        vy_ = -vy_;

    const int16_t x = bx_ / kFixed;
    if (vy_ > 0 && by_ / kFixed >= kPaddleY - 2 && x >= paddleX_ - 1 && x <= paddleX_ + kPaddleW)
        bounceOnPaddle();

    if (by_ / kFixed > 63) {
        if (--lives_)
            serve();
    }

    if (left_ == 0) {
        ++level_;
        buildBricks();
        serve();
    }
}

void Breakout::draw(mrm::Panel& o, uint32_t now) const {
    (void)now;
    hud(o, score_, lives_);

    for (uint8_t r = 0; r < kBrickRows; ++r) {
        for (uint8_t c = 0; c < kBrickCols; ++c) {
            if (bricks_[r] & (1u << c))
                o.fillRect(kBrickLeft + c * kBrickPitchX, kBrickTop + r * kBrickPitchY, kBrickW, kBrickH);
        }
    }

    o.fillRect(paddleX_, kPaddleY, kPaddleW, 2);
    o.fillRect(bx_ / kFixed, by_ / kFixed, 2, 2);
}

// Uma bola quica e a raquete a segue. A acende quando a raquete vai para a esquerda e B para a direita.
uint8_t Breakout::demo(mrm::Panel& o, uint32_t now) const {
    const uint32_t t = now / 30;
    const int16_t span = 100;
    const int16_t phase = t % (span * 2);
    const int16_t ballX = 10 + (phase < span ? phase : span * 2 - phase);
    const int16_t ballY = 30 + (t % 36 < 18 ? t % 18 : 18 - t % 18);
    for (uint8_t c = 0; c < 9; ++c)
        o.fillRect(8 + c * 12, 27, kBrickW, 3);

    o.fillRect(ballX, ballY, 2, 2);
    o.fillRect(ballX - 10, 46, kPaddleW, 2);
    return phase < span ? kLitB : kLitA;
}

} // namespace twobtn
} // namespace game
} // namespace mrm
