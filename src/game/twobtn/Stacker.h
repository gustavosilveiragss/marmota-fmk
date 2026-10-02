#pragma once

#include "../Game.h"

namespace mrm {
namespace game {
namespace twobtn {

// Torre: um bloco vai e volta, A solta, B freia (3 por partida). A sobra cai e o proximo vem menor.
class Stacker : public Game {
public:
    const GameText& text() const override;
    void reset(uint32_t seed) override;
    void tick(const GameInput& in) override;
    void draw(mrm::Panel& o, uint32_t now) const override;
    uint8_t demo(mrm::Panel& o, uint32_t now) const override;
    bool over() const override { return over_; }
    uint16_t score() const override { return score_; }

private:
    static constexpr uint8_t kRing = 13; // andares guardados: mais que os visiveis

    void drop();
    void startBlock();
    int16_t cameraBase() const { return floors_ > 10 ? floors_ - 10 : 0; }
    int16_t floorY(int16_t floor) const { return 60 - (floor - cameraBase()) * 4; }

    uint8_t x_[kRing];
    uint8_t w_[kRing];
    int16_t pos_ = 0; // bloco atual em 1/16 de pixel
    int16_t vel_ = 0;
    uint16_t floors_ = 0; // andares empilhados, o bloco atual e o proximo
    uint16_t score_ = 0;
    uint8_t blockW_ = 0;
    uint8_t brakes_ = 0;
    bool braked_ = false;
    bool over_ = false;
    uint8_t cutX_ = 0; // sobra em queda
    uint8_t cutW_ = 0;
    uint8_t cutAge_ = 0;
    int16_t cutFloor_ = 0;
    uint32_t rng_ = 1;
};

} // namespace twobtn
} // namespace game
} // namespace mrm
