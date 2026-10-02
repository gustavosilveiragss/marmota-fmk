#pragma once

#include "../Game.h"

namespace mrm {
namespace game {
namespace twobtn {

// Ritmo: notas descem em duas pistas, A toca a da esquerda e B a da direita; segurar sustenta notas longas.
class Rhythm : public Game {
public:
    const GameText& text() const override;
    void reset(uint32_t seed) override;
    void tick(const GameInput& in) override;
    void draw(mrm::Panel& o, uint32_t now) const override;
    uint8_t demo(mrm::Panel& o, uint32_t now) const override;
    bool over() const override { return lives_ == 0; }
    uint16_t score() const override { return score_; }

private:
    static constexpr uint8_t kQueue = 16;
    struct Note {
        int8_t t;     // ticks ate a cabeca cruzar a linha (negativo: ja passou)
        uint8_t info; // bit 0 pista, bits 1 a 5 duracao em ticks, bit 6 acertada, bit 7 encerrada
    };

    static bool lane(const Note& n) { return n.info & 1; }
    static uint8_t length(const Note& n) { return (n.info >> 1) & 31; }
    static bool done(const Note& n) { return n.info & 0x80; }
    static bool hit(const Note& n) { return n.info & 0x40; }

    void spawn();
    void planNext();
    void press(uint8_t pista);
    void sustain(const bool down[2]);
    void age();
    void miss();
    void award(int8_t distance);

    Note notes_[kQueue];
    uint8_t first_ = 0;
    uint8_t count_ = 0;
    int16_t gap_ = 0; // ticks ate a proxima nota planejada cruzar a linha
    uint8_t nextLane_ = 0;
    uint8_t nextLen_ = 0;
    uint16_t planned_ = 0;
    uint16_t score_ = 0;
    uint8_t combo_ = 0;
    uint8_t lives_ = 0;
    uint8_t flash_[2] = {0, 0};
    bool down_[2] = {false, false};
    uint32_t rng_ = 1;
};

} // namespace twobtn
} // namespace game
} // namespace mrm
