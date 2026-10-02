#pragma once

#include "../Game.h"

namespace mrm {
namespace game {
namespace twobtn {

// Orbita: um ponto gira em volta do centro, A para um lado e B para o outro; passe pela brecha dos aneis.
class Orbit : public Game {
public:
    const GameText& text() const override;
    void reset(uint32_t seed) override;
    void tick(const GameInput& in) override;
    void draw(mrm::Panel& o, uint32_t now) const override;
    uint8_t demo(mrm::Panel& o, uint32_t now) const override;
    bool over() const override { return over_; }
    uint16_t score() const override { return score_; }

private:
    static constexpr uint8_t kRings = 4;
    struct Ring {
        uint16_t r16;  // raio em 1/16 de pixel
        uint16_t gaps; // bit k: setor k aberto (16 setores)
        bool on;
    };

    void move(const GameInput& in);
    void advance();
    void spawn();
    bool blocked(const Ring& ring) const;
    uint16_t makeGaps();

    Ring rings_[kRings];
    uint8_t angle_ = 0; // 256 passos por volta
    uint16_t score_ = 0;
    bool over_ = false;
    uint32_t rng_ = 1;
};

} // namespace twobtn
} // namespace game
} // namespace mrm
