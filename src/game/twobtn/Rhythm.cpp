#include "Rhythm.h"

#include "Text.h"

namespace mrm {
namespace game {
namespace twobtn {

namespace {

constexpr int16_t kLineY = 54;
constexpr int16_t kPxPerTick = 2;
constexpr int8_t kFall = 21; // ticks do topo até a linha
constexpr int8_t kExact = 3;
constexpr int8_t kNear = 6;
constexpr uint8_t kLives = 3;
constexpr uint8_t kNotesPerStep = 16;
constexpr uint8_t kLongFrom = 30;
constexpr uint8_t kStartInterval = 17;
constexpr uint8_t kMinInterval = 8;
constexpr uint8_t kComboDouble = 10;
constexpr uint8_t kHoldEvery = 6;
constexpr int16_t kLaneX[2] = {32, 96};
constexpr int16_t kNoteW = 28;
constexpr GameText kText = {
    Str::GameRhythm, Str::GoalRhythm, Str::KeyLeft, Str::KeyRight, {Str::RhythmR1, Str::RhythmR2, Str::RhythmR3}};

} // namespace

const GameText& Rhythm::text() const {
    return kText;
}

void Rhythm::reset(uint32_t seed) {
    rng_ = seed | 1;
    first_ = count_ = 0;
    planned_ = 0;

    score_ = 0;
    combo_ = 0;
    lives_ = kLives;

    flash_[0] = flash_[1] = 0;
    down_[0] = down_[1] = false;

    gap_ = kFall + 30;
    planNext();
}

// Sorteia a próxima nota. Só entram notas longas depois de kLongFrom, e nunca nas duas pistas ao mesmo tempo.
void Rhythm::planNext() {
    const uint32_t r = nextRandom(rng_);
    nextLane_ = (r >> 8) & 1;
    nextLen_ = planned_ >= kLongFrom && (r >> 12) % 5 == 0 ? 8 + (r >> 16) % 12 : 0;
}

void Rhythm::spawn() {
    const uint8_t step = min<uint16_t>(planned_ / kNotesPerStep, kStartInterval - kMinInterval);
    const int16_t interval = kStartInterval - step;
    while (gap_ <= kFall + nextLen_ && count_ < kQueue) {
        notes_[(first_ + count_) % kQueue] = {static_cast<int8_t>(gap_),
                                              static_cast<uint8_t>(nextLane_ | nextLen_ << 1)};

        ++count_;
        ++planned_;
        gap_ += nextLen_ + interval + nextRandom(rng_) % 4;
        planNext();
    }
}

void Rhythm::miss() {
    combo_ = 0;

    if (lives_)
        --lives_;
}

// Pontos de um acerto: exato vale 2, perto vale 1, e o combo dobra.
void Rhythm::award(int8_t distance) {
    const uint8_t base = abs(distance) <= kExact ? 2 : 1;
    score_ += combo_ >= kComboDouble ? base * 2 : base;

    if (combo_ < 255)
        ++combo_;
}

void Rhythm::press(uint8_t pista) {
    Note* best = nullptr;
    for (uint8_t i = 0; i < count_; ++i) {
        Note& n = notes_[(first_ + i) % kQueue];
        if (!done(n) && !hit(n) && lane(n) == pista && abs(n.t) <= kNear && (!best || abs(n.t) < abs(best->t)))
            best = &n;
    }

    if (!best) {
        miss();
        return;
    }

    award(best->t);
    flash_[pista] = 4;
    best->info |= length(*best) ? 0x40 : 0x80;
}

// Nota longa segurada pontua a cada kHoldEvery ticks. Soltar antes só para de pontuar.
void Rhythm::sustain(const bool down[2]) {
    for (uint8_t i = 0; i < count_; ++i) {
        Note& n = notes_[(first_ + i) % kQueue];
        if (!hit(n) || done(n))
            continue;

        if (!down[lane(n)] || n.t <= -static_cast<int8_t>(length(n))) {
            n.info |= 0x80;
        } else if (n.t <= 0 && n.t % kHoldEvery == 0) {
            ++score_;
        }
    }
}

void Rhythm::age() {
    for (uint8_t i = 0; i < count_; ++i) {
        Note& n = notes_[(first_ + i) % kQueue];
        --n.t;

        if (!done(n) && !hit(n) && n.t < -kNear) {
            n.info |= 0x80;
            miss();
        }
    }

    while (count_ && done(notes_[first_]) && notes_[first_].t < -static_cast<int8_t>(length(notes_[first_])) - 2) {
        first_ = (first_ + 1) % kQueue;
        --count_;
    }
}

void Rhythm::tick(const GameInput& in) {
    --gap_;
    age();
    spawn();

    if (in.aPress)
        press(0);

    if (in.bPress)
        press(1);

    const bool down[2] = {in.a, in.b};
    sustain(down);
    down_[0] = in.a;
    down_[1] = in.b;

    for (uint8_t& f : flash_) {
        if (f)
            --f;
    }
}

void Rhythm::draw(mrm::Panel& o, uint32_t now) const {
    (void)now;
    hud(o, score_, lives_);

    for (uint8_t p = 0; p < 2; ++p) {
        const int16_t x = kLaneX[p] - kNoteW / 2;
        for (int16_t y = kFieldTop + 1; y < kLineY; y += 4)
            o.setPixel(kLaneX[p], y);

        if (flash_[p] || down_[p])
            o.fillRect(x, kLineY, kNoteW, 4);
        else
            o.drawRect(x, kLineY, kNoteW, 4);
    }

    for (uint8_t i = 0; i < count_; ++i) {
        const Note& n = notes_[(first_ + i) % kQueue];
        if (done(n))
            continue;

        const int16_t head = hit(n) ? min<int16_t>(kLineY - n.t * kPxPerTick, kLineY) : kLineY - n.t * kPxPerTick;
        const int16_t x = kLaneX[lane(n)] - kNoteW / 2;
        if (length(n)) {
            const int16_t tail = max<int16_t>(kLineY - n.t * kPxPerTick - length(n) * kPxPerTick, kFieldTop);
            if (head > tail)
                hit(n) ? o.fillRect(x + 8, tail, kNoteW - 16, head - tail)
                       : o.drawRect(x + 8, tail, kNoteW - 16, head - tail);
        }

        if (head >= kFieldTop && head < 62)
            o.fillRect(x, head, kNoteW, 4);
    }

    if (combo_ >= 2) {
        char text[6];
        snprintf(text, sizeof(text), "x%u", combo_);
        o.setFont(ArialMT_Plain_10);
        o.setTextAlignment(TEXT_ALIGN_CENTER);
        o.drawText(64, 14, text);
    }
}

// Três notas: A toca a primeira, B a segunda e A segura uma longa.
uint8_t Rhythm::demo(mrm::Panel& o, uint32_t now) const {
    constexpr uint32_t kLoop = 3400;
    constexpr int16_t kLine = 44;
    constexpr int16_t kSpan = 18; // pixels percorridos em 700 ms
    const uint32_t t = now % kLoop;
    uint8_t lit = 0;
    for (uint8_t p = 0; p < 2; ++p) {
        o.drawRect(kLaneX[p] - 14, kLine, kNoteW, 4);
    }

    const auto tap = [&](uint32_t at, uint8_t pista, uint8_t key) {
        const int32_t dt = static_cast<int32_t>(at) - static_cast<int32_t>(t); // ms até cruzar
        const int16_t y = kLine - dt * kSpan / 700;
        if (dt > -80 && y >= 26)
            o.fillRect(kLaneX[pista] - 14, y, kNoteW, 3);

        if (dt > -150 && dt < 100)
            lit |= key;
    };

    tap(900, 0, kLitA);
    tap(1600, 1, kLitB);

    const int32_t dt = 2300 - static_cast<int32_t>(t);
    const int16_t head = kLine - dt * kSpan / 700;
    const int16_t tail = max<int16_t>(head - 16, 26);
    const int16_t bottom = min<int16_t>(head, kLine);
    if (head >= 26 && bottom > tail) {
        o.drawRect(kLaneX[0] - 6, tail, 12, bottom - tail + 1);
        o.fillRect(kLaneX[0] - 14, bottom, kNoteW, 3);
    }

    if (t >= 2200 && t < 2920)
        lit |= kLitA;

    return lit;
}

} // namespace twobtn
} // namespace game
} // namespace mrm
