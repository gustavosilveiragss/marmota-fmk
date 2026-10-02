#include "Stacker.h"

#include "Text.h"

namespace mrm {
namespace game {
namespace twobtn {

namespace {

constexpr int16_t kFixed = 16;
constexpr uint8_t kStartW = 48;
constexpr uint8_t kMaxW = 56;
constexpr uint8_t kBrakes = 3;
constexpr int16_t kBaseSpeed = 24; // 1,5 px por passo
constexpr int16_t kMaxSpeed = 80;
constexpr uint8_t kFloorsPerLevel = 5;
constexpr uint8_t kRandomSideFrom = 20;
constexpr uint8_t kPerfectSlack = 1;
constexpr uint8_t kCutTicks = 14;
constexpr GameText kText = {txt(Str::GameStacker), txt(Str::GoalStacker), txt(Str::KeyDrop), txt(Str::KeyBrake), {txt(Str::StackerR1), txt(Str::StackerR2), txt(Str::StackerR3)}};

} // namespace

const GameText& Stacker::text() const {
    return kText;
}

void Stacker::reset(uint32_t seed) {
    rng_ = seed | 1;
    x_[0] = (128 - kStartW) / 2;
    w_[0] = kStartW;
    floors_ = 1;
    score_ = 0;
    blockW_ = kStartW;
    brakes_ = kBrakes;
    over_ = false;
    cutAge_ = kCutTicks;
    startBlock();
}

void Stacker::startBlock() {
    const bool right = floors_ >= kRandomSideFrom && (nextRandom(rng_) & 1);
    const int16_t speed = min<int16_t>(kBaseSpeed + (floors_ / kFloorsPerLevel) * 4, kMaxSpeed);
    pos_ = right ? (128 - blockW_) * kFixed : 0;
    vel_ = right ? -speed : speed;
    braked_ = false;
}

void Stacker::tick(const GameInput& in) {
    if (cutAge_ < kCutTicks)
        ++cutAge_;
    if (in.bPress && brakes_ && !braked_) {
        --brakes_;
        braked_ = true;
    }
    if (in.aPress) {
        drop();
        return;
    }
    pos_ += braked_ ? vel_ / 2 : vel_;
    const int16_t limit = (128 - blockW_) * kFixed;
    if (pos_ < 0 || pos_ > limit) {
        vel_ = -vel_;
        pos_ = constrain(pos_, 0, limit);
    }
}

void Stacker::drop() {
    const uint8_t top = (floors_ - 1) % kRing;
    const int16_t bx = pos_ / kFixed;
    const int16_t px = x_[top];
    const int16_t pw = w_[top];
    const int16_t left = max<int16_t>(bx, px);
    const int16_t right = min<int16_t>(bx + blockW_, px + pw);
    if (right <= left) {
        over_ = true;
        return;
    }
    const uint8_t slot = floors_ % kRing;
    ++score_;
    if (abs(bx - px) <= kPerfectSlack && blockW_ <= pw) {
        score_ += 2;
        blockW_ = min<uint8_t>(blockW_ + 2, kMaxW);
        x_[slot] = constrain(px - 1, 0, 128 - blockW_);
        w_[slot] = blockW_;
        cutAge_ = kCutTicks;
    } else {
        cutX_ = bx < px ? bx : right;
        cutW_ = blockW_ - (right - left);
        cutAge_ = 0;
        cutFloor_ = floors_;
        x_[slot] = left;
        w_[slot] = right - left;
        blockW_ = right - left;
    }
    ++floors_;
    startBlock();
}

void Stacker::draw(mrm::Panel& o, uint32_t now) const {
    (void)now;
    hud(o, score_, brakes_);
    const int16_t base = cameraBase();
    for (int16_t f = base; f < floors_; ++f)
        o.fillRect(x_[f % kRing], floorY(f), w_[f % kRing], 3);
    o.fillRect(pos_ / kFixed, floorY(floors_), blockW_, 3);
    const int16_t cutY = floorY(cutFloor_) + cutAge_ * cutAge_ / 6;
    if (cutAge_ < kCutTicks && cutFloor_ >= base && cutY < 62)
        o.fillRect(cutX_, cutY, cutW_, 3);
}

// Tres andares e um bloco que desliza; A solta em x 68, a sobra cai e o proximo bloco comeca menor.
uint8_t Stacker::demo(mrm::Panel& o, uint32_t now) const {
    constexpr uint32_t kLoop = 3000;
    constexpr uint32_t kDropAt = 1500;
    const uint32_t t = now % kLoop;
    o.fillRect(40, 44, 48, 3);
    o.fillRect(44, 40, 40, 3);
    o.fillRect(48, 36, 32, 3);
    if (t < kDropAt) {
        o.fillRect(20 + t / 31, 32, 24, 3);
    } else {
        o.fillRect(68, 32, 12, 3);
        o.fillRect(80, min<int16_t>(32 + (t - kDropAt) / 25, 45), 12, 3);
        if (t >= kDropAt + 600)
            o.fillRect(20 + (t - kDropAt - 600) / 40, 28, 12, 3);
    }
    return t >= kDropAt - 120 && t < kDropAt + 120 ? kLitA : 0;
}

} // namespace twobtn
} // namespace game
} // namespace mrm
