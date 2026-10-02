#include "Orbit.h"

#include "Text.h"

namespace mrm {
namespace game {
namespace twobtn {

namespace {

constexpr int16_t kCx = 64;
constexpr int16_t kCy = 37;
constexpr int16_t kOuter = 24;
constexpr int16_t kPlayerR = 14;
constexpr int16_t kSpacing = 13; // px entre aneis
constexpr uint8_t kHoldStep = 4;
constexpr uint8_t kTapStep = 8;
constexpr uint8_t kDot = 2; // meia largura do ponto, em passos de 1/256
constexpr uint8_t kFixed = 16;
constexpr uint8_t kBaseSpeed = 6;
constexpr uint8_t kMaxSpeed = 12;
constexpr uint16_t kDoubleFrom = 20;
constexpr int8_t kQuarter[33] = {0, 6, 12, 19, 25, 31, 37, 43, 49, 54, 60, 65, 71, 76, 81, 85, 90, 94, 98, 102, 106, 109, 112, 115, 117, 120, 122, 123, 125, 126, 126, 127, 127};
constexpr GameText kText = {txt(Str::GameOrbit), txt(Str::GoalOrbit), txt(Str::KeyLeft), txt(Str::KeyRight), {txt(Str::OrbitR1), txt(Str::OrbitR2), txt(Str::OrbitR3)}};

// Seno em 128 passos por volta, de -127 a 127.
int16_t sine(uint8_t step) {
    const uint8_t quarter = (step >> 5) & 3;
    const uint8_t r = step & 31;
    const int16_t v = kQuarter[quarter & 1 ? 32 - r : r];
    return quarter >= 2 ? -v : v;
}

int16_t cosine(uint8_t step) {
    return sine(step + 32);
}

} // namespace

const GameText& Orbit::text() const {
    return kText;
}

void Orbit::reset(uint32_t seed) {
    rng_ = seed | 1;
    for (Ring& ring : rings_)
        ring.on = false;
    angle_ = 8;
    score_ = 0;
    over_ = false;
    spawn();
}

// A brecha nasce perto de onde o ponto esta (o anel anterior ja passou): o salto cabe no tempo
// que o anel novo leva do topo ate a zona do ponto.
uint16_t Orbit::makeGaps() {
    const uint16_t level = score_ / 10;
    const uint16_t speed = min<uint16_t>(kBaseSpeed + level, kMaxSpeed);
    const int16_t ticks = (kOuter - kPlayerR - 2) * kFixed / speed;
    const int16_t reach = constrain(ticks * kHoldStep / 16 - 1, 2, 7);
    const bool twin = score_ >= kDoubleFrom && (nextRandom(rng_) & 1);
    const uint8_t width = twin ? 2 : max<int16_t>(3, 6 - level);
    const int16_t shift = static_cast<int16_t>(nextRandom(rng_) % (2 * reach + 1)) - reach;
    const uint8_t first = (angle_ / 16 + 16 + shift - width / 2) & 15;
    uint16_t gaps = 0;
    for (uint8_t k = 0; k < width; ++k) {
        gaps |= 1u << ((first + k) & 15);
        if (twin)
            gaps |= 1u << ((first + k + 8) & 15);
    }
    return gaps;
}

void Orbit::spawn() {
    for (Ring& ring : rings_) {
        if (!ring.on) {
            ring = {static_cast<uint16_t>(kOuter * kFixed), makeGaps(), true};
            return;
        }
    }
}

void Orbit::move(const GameInput& in) {
    if (in.aPress)
        angle_ -= kTapStep;
    else if (in.a)
        angle_ -= kHoldStep;
    if (in.bPress)
        angle_ += kTapStep;
    else if (in.b)
        angle_ += kHoldStep;
}

// O ponto precisa estar dentro da brecha nas duas bordas.
bool Orbit::blocked(const Ring& ring) const {
    const uint8_t lo = static_cast<uint8_t>(angle_ - kDot) >> 4;
    const uint8_t hi = static_cast<uint8_t>(angle_ + kDot) >> 4;
    return !((ring.gaps >> lo) & 1) || !((ring.gaps >> hi) & 1);
}

void Orbit::advance() {
    const uint16_t speed = min<uint16_t>(kBaseSpeed + score_ / 10, kMaxSpeed);
    bool needNew = true;
    for (Ring& ring : rings_) {
        if (!ring.on)
            continue;
        ring.r16 -= min<uint16_t>(speed, ring.r16);
        const int16_t r = ring.r16 / kFixed;
        if (abs(r - kPlayerR) <= 2 && blocked(ring))
            over_ = true;
        if (r < kPlayerR - 3) {
            ring.on = false;
            ++score_;
        } else if (ring.r16 > (kOuter - kSpacing) * kFixed) {
            needNew = false;
        }
    }
    if (needNew)
        spawn();
}

void Orbit::tick(const GameInput& in) {
    move(in);
    advance();
}

void Orbit::draw(mrm::Panel& o, uint32_t now) const {
    (void)now;
    hud(o, score_, 0);
    o.setPixel(kCx, kCy);
    for (const Ring& ring : rings_) {
        if (!ring.on)
            continue;
        const int16_t r = ring.r16 / kFixed;
        for (uint8_t i = 0; i < 128; ++i) {
            if ((ring.gaps >> (i >> 3)) & 1)
                continue;
            o.fillRect(kCx + cosine(i) * r / 127 - 1, kCy + sine(i) * r / 127 - 1, 2, 2);
        }
    }
    const uint8_t step = angle_ >> 1;
    o.fillRect(kCx + cosine(step) * kPlayerR / 127 - 1, kCy + sine(step) * kPlayerR / 127 - 1, 3, 3);
}

// Um anel fecha com a brecha a direita; B gira o ponto ate ela, depois A volta para a outra brecha.
uint8_t Orbit::demo(mrm::Panel& o, uint32_t now) const {
    constexpr uint32_t kPhase = 2400;
    constexpr int16_t kPlayer = 6;
    const uint32_t t = now % (kPhase * 2);
    const bool second = t >= kPhase;
    const uint32_t p = t % kPhase;
    const int16_t r = 10 - static_cast<int16_t>(p * 6 / 1900);
    o.setPixel(kCx, kCy);
    if (p < 1900) {
        for (uint8_t i = 0; i < 128; ++i) {
            const uint8_t d = i < 64 ? i : 128 - i;
            if (d > 10)
                o.fillRect(kCx + cosine(i) * r / 127, kCy + sine(i) * r / 127, 2, 2);
        }
    }
    const int32_t move = min<uint32_t>(p, 1300) * 28 / 1300;
    const uint8_t step = second ? 28 - move : 100 + move;
    o.fillRect(kCx + cosine(step) * kPlayer / 127 - 1, kCy + sine(step) * kPlayer / 127 - 1, 3, 3);
    if (p >= 1300)
        return 0;
    return second ? kLitA : kLitB;
}

} // namespace twobtn
} // namespace game
} // namespace mrm
