#pragma once

#include "../Game.h"

namespace mrm {
namespace game {
namespace twobtn {

// Space Impact do Nokia: A sobe a nave, B desce, o tiro e automático. Ondas de naves e um chefe.
class Impact : public Game {
public:
    const GameText& text() const override;
    void reset(uint32_t seed) override;
    void tick(const GameInput& in) override;
    void draw(mrm::Panel& o, uint32_t now) const override;
    uint8_t demo(mrm::Panel& o, uint32_t now) const override;
    bool over() const override { return lives_ == 0; }
    uint16_t score() const override { return score_; }

private:
    struct Bullet {
        int16_t x;
        int16_t y;
        bool on;
    };
    struct Foe {
        int16_t x;
        int16_t y;
        int16_t baseY;
        uint8_t kind; // 0 reta, 1 onda, 2 mergulho, 3 chefe
        uint8_t hp;
        uint8_t age;
        bool on;
    };

    void moveShip(const GameInput& in);
    void fire();
    void shoot(int16_t y);
    void moveBullets();
    void spawn();
    void spawnFoe(uint8_t kind);
    void moveFoes();
    void moveFoe(Foe& foe);
    void bossShoot(const Foe& foe);
    void collide();
    void hitShip();
    void killFoe(Foe& foe);
    Foe* boss();
    struct Box {
        int16_t x;
        int16_t y;
        int16_t w;
        int16_t h;
    };
    static bool overlap(const Box& a, const Box& b);

    static constexpr uint8_t kShots = 6;
    static constexpr uint8_t kFoes = 8;
    static constexpr uint8_t kFoeShots = 5;

    Bullet shots_[kShots];
    Bullet foeShots_[kFoeShots];
    Foe foes_[kFoes];
    Bullet drop_;
    int16_t shipY_ = 36;
    uint16_t score_ = 0;
    uint8_t lives_ = 0;
    uint8_t wave_ = 0;
    uint8_t spawned_ = 0;
    uint8_t kills_ = 0;
    uint8_t fireTimer_ = 0;
    uint8_t spawnTimer_ = 0;
    uint8_t bossTimer_ = 0;
    uint8_t invulnerable_ = 0;
    uint16_t doubleShot_ = 0;
    uint32_t rng_ = 1;
};

} // namespace twobtn
} // namespace game
} // namespace mrm
