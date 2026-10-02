#include "Impact.h"

#include "Text.h"

namespace mrm {
namespace game {
namespace twobtn {

namespace {

constexpr int16_t kShipX = 4;
constexpr int16_t kShipW = 8;
constexpr int16_t kShipH = 5;
constexpr int16_t kBottom = 63;
constexpr int16_t kBossW = 14;
constexpr int16_t kBossX = 110;
constexpr uint8_t kFireEvery = 8;
constexpr uint8_t kBossEvery = 3;      // a cada 3 ondas
constexpr uint8_t kPerWave = 10;       // naves comuns por onda
constexpr uint8_t kDropEvery = 8;      // abates por item de tiro duplo
constexpr uint16_t kDoubleTicks = 330; // 10 s
constexpr uint8_t kInvulnerableTicks = 60;
constexpr GameText kText = {txt(Str::GameImpact), txt(Str::GoalImpact), txt(Str::KeyUp), txt(Str::KeyDown), {txt(Str::ImpactR1), txt(Str::ImpactR2), txt(Str::ImpactR3)}};

} // namespace

const GameText& Impact::text() const {
    return kText;
}

bool Impact::overlap(const Box& a, const Box& b) {
    return a.x < b.x + b.w && b.x < a.x + a.w && a.y < b.y + b.h && b.y < a.y + a.h;
}

void Impact::reset(uint32_t seed) {
    memset(shots_, 0, sizeof(shots_));
    memset(foeShots_, 0, sizeof(foeShots_));
    memset(foes_, 0, sizeof(foes_));
    drop_ = {};
    rng_ = seed | 1;
    shipY_ = 36;
    score_ = 0;
    lives_ = 3;
    wave_ = spawned_ = kills_ = bossTimer_ = 0;
    fireTimer_ = 0;
    spawnTimer_ = 30;
    invulnerable_ = 0;
    doubleShot_ = 0;
}

void Impact::tick(const GameInput& in) {
    moveShip(in);
    fire();
    moveBullets();
    spawn();
    moveFoes();
    collide();
    if (invulnerable_)
        --invulnerable_;
    if (doubleShot_)
        --doubleShot_;
}

void Impact::moveShip(const GameInput& in) {
    if (in.a)
        shipY_ -= 2;
    if (in.b)
        shipY_ += 2;
    shipY_ = constrain(shipY_, kFieldTop + 1, kBottom - kShipH);
}

void Impact::shoot(int16_t y) {
    for (Bullet& s : shots_) {
        if (!s.on) {
            s = {kShipX + kShipW, y, true};
            return;
        }
    }
}

void Impact::fire() {
    if (fireTimer_ && --fireTimer_)
        return;
    fireTimer_ = kFireEvery;
    if (doubleShot_) {
        shoot(shipY_ - 1);
        shoot(shipY_ + kShipH);
    } else {
        shoot(shipY_ + kShipH / 2);
    }
}

void Impact::moveBullets() {
    for (Bullet& s : shots_) {
        if (s.on && (s.x += 4) > 127)
            s.on = false;
    }
    for (Bullet& s : foeShots_) {
        if (s.on && (s.x -= 2) < 0)
            s.on = false;
    }
    if (drop_.on && (drop_.x -= 1) < 0)
        drop_.on = false;
}

Impact::Foe* Impact::boss() {
    for (Foe& f : foes_) {
        if (f.on && f.kind == 3)
            return &f;
    }
    return nullptr;
}

void Impact::spawnFoe(uint8_t kind) {
    for (Foe& f : foes_) {
        if (f.on)
            continue;
        const int16_t y = kFieldTop + 4 + nextRandom(rng_) % (kBottom - kFieldTop - 14);
        f = {127, y, y, kind, static_cast<uint8_t>(kind == 3 ? 12 + wave_ * 2 : 1), 0, true};
        if (kind == 3) {
            f.x = kBossX;
            f.y = f.baseY = kFieldTop + 20;
        }
        return;
    }
}

void Impact::spawn() {
    if (boss())
        return; // com o chefe em campo nao entram naves comuns
    if (spawnTimer_ && --spawnTimer_)
        return;
    spawnTimer_ = max<int>(12, 40 - wave_ * 3);
    if (spawned_ >= kPerWave) {
        spawned_ = 0;
        ++wave_;
        if (wave_ % kBossEvery == 0) {
            spawnFoe(3);
            return;
        }
    }
    ++spawned_;
    spawnFoe(nextRandom(rng_) % (wave_ ? 3 : 2));
}

void Impact::bossShoot(const Foe& foe) {
    for (Bullet& s : foeShots_) {
        if (!s.on) {
            s = {static_cast<int16_t>(foe.x - 2), static_cast<int16_t>(foe.y + kBossW / 2), true};
            return;
        }
    }
}

void Impact::moveFoe(Foe& f) {
    ++f.age;
    switch (f.kind) {
    case 0:
        f.x -= 1 + wave_ / 5;
        break;
    case 1:
        f.x -= 1;
        f.y = f.baseY + static_cast<int16_t>(sinf(f.age / 6.0f) * 10);
        break;
    case 2:
        f.x -= 2;
        f.y += f.y < shipY_ ? 1 : (f.y > shipY_ ? -1 : 0);
        break;
    default: // chefe: sobe e desce e atira
        f.y = f.baseY + static_cast<int16_t>(sinf(f.age / 14.0f) * 16);
        if (++bossTimer_ >= 36) {
            bossTimer_ = 0;
            bossShoot(f);
        }
        break;
    }
    if (f.x < -kBossW)
        f.on = false;
}

void Impact::moveFoes() {
    for (Foe& f : foes_) {
        if (f.on)
            moveFoe(f);
    }
}

void Impact::killFoe(Foe& foe) {
    foe.on = false;
    const bool wasBoss = foe.kind == 3;
    score_ += wasBoss ? 50 : 10;
    if (wasBoss) {
        memset(foeShots_, 0, sizeof(foeShots_));
        spawnTimer_ = 20;
    }
    if (++kills_ % kDropEvery == 0 && !drop_.on)
        drop_ = {foe.x, foe.y, true};
}

void Impact::hitShip() {
    if (invulnerable_)
        return;
    --lives_;
    invulnerable_ = kInvulnerableTicks;
    doubleShot_ = 0;
}

void Impact::collide() {
    const Box ship{kShipX, shipY_, kShipW, kShipH};
    for (Foe& f : foes_) {
        if (!f.on)
            continue;
        const int16_t size = f.kind == 3 ? kBossW : 7;
        const Box body{f.x, f.y, size, static_cast<int16_t>(f.kind == 3 ? kBossW : 5)};
        for (Bullet& s : shots_) {
            if (s.on && overlap({s.x, s.y, 3, 1}, body)) {
                s.on = false;
                if (--f.hp == 0) {
                    killFoe(f);
                    break;
                }
            }
        }
        if (f.on && overlap(ship, body)) {
            hitShip();
            if (f.kind != 3)
                f.on = false;
        }
    }
    for (Bullet& s : foeShots_) {
        if (s.on && overlap(ship, {s.x, s.y, 2, 2})) {
            s.on = false;
            hitShip();
        }
    }
    if (drop_.on && overlap(ship, {drop_.x, drop_.y, 5, 5})) {
        drop_.on = false;
        doubleShot_ = kDoubleTicks;
    }
}

void Impact::draw(mrm::Panel& o, uint32_t now) const {
    hud(o, score_, lives_);
    if (!invulnerable_ || (now / 90) % 2)
        o.fillTriangle(kShipX, shipY_, kShipX, shipY_ + kShipH - 1, kShipX + kShipW - 1, shipY_ + kShipH / 2);
    for (const Bullet& s : shots_) {
        if (s.on)
            o.fillRect(s.x, s.y, 3, 1);
    }
    for (const Bullet& s : foeShots_) {
        if (s.on)
            o.fillRect(s.x, s.y, 2, 2);
    }
    for (const Foe& f : foes_) {
        if (!f.on)
            continue;
        if (f.kind == 0)
            o.drawRect(f.x, f.y, 7, 5);
        else if (f.kind == 1)
            o.drawCircle(f.x + 3, f.y + 2, 3);
        else if (f.kind == 2)
            o.fillTriangle(f.x + 6, f.y, f.x + 6, f.y + 4, f.x, f.y + 2);
        else {
            o.drawRect(f.x, f.y, kBossW, kBossW);
            o.fillRect(f.x + 3, f.y + 3, kBossW - 6, kBossW - 6);
        }
    }
    if (drop_.on) {
        o.drawRect(drop_.x, drop_.y, 5, 5);
        o.drawHorizontalLine(drop_.x + 1, drop_.y + 2, 3);
    }
}

// A nave sobe e desce sozinha atirando; uma nave inimiga cruza e some quando acertada.
uint8_t Impact::demo(mrm::Panel& o, uint32_t now) const {
    const uint32_t t = now / 40;
    const int16_t span = 14;
    const uint32_t cycle = t % (span * 2 * 2);
    const bool up = (cycle % (span * 2)) >= span; // 1a metade desce (B), 2a sobe (A)
    const int16_t pos = (cycle % (span * 2)) < span ? cycle % (span * 2) : span * 2 - cycle % (span * 2);
    const int16_t y = 28 + pos;
    o.fillTriangle(4, y, 4, y + 4, 11, y + 2);
    const int16_t bulletX = 14 + (t * 4) % 90;
    o.fillRect(bulletX, y + 2, 3, 1);
    const int16_t foeX = 118 - (t * 2) % 110;
    o.drawRect(foeX, 33, 7, 5);
    return up ? kLitA : kLitB;
}

} // namespace twobtn
} // namespace game
} // namespace mrm
