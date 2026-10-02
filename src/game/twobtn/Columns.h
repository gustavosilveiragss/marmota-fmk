#pragma once

#include "../Game.h"

namespace mrm {
namespace game {
namespace twobtn {

// Colunas: cai uma peca de 3 simbolos. A anda uma coluna para a direita (com volta), toque em B gira,
// B segurado derruba. Tres simbolos iguais em linha, coluna ou diagonal somem.
class Columns : public Game {
public:
    const GameText& text() const override;
    void reset(uint32_t seed) override;
    void tick(const GameInput& in) override;
    void draw(mrm::Panel& o, uint32_t now) const override;
    uint8_t demo(mrm::Panel& o, uint32_t now) const override;
    bool over() const override { return over_; }
    uint16_t score() const override { return score_; }

private:
    static constexpr uint8_t kCols = 6;
    static constexpr uint8_t kRows = 6;

    uint8_t randomSymbol();
    void newPiece();
    bool fits(uint8_t col) const;
    void moveRight();
    void rotate();
    void handleB(bool down);
    void fall();
    void lock();
    void resolve();
    void clearMarked();
    uint8_t markMatches();
    void markRun(int8_t r, int8_t c, int8_t dr, int8_t dc);
    bool marked(uint8_t r, uint8_t c) const { return mark_[r] & (1u << c); }

    uint8_t grid_[kRows][kCols]; // 0 vazio, 1 a 4 simbolos
    uint8_t mark_[kRows];        // bit c: celula marcada para sumir
    uint8_t piece_[3];           // de cima para baixo
    uint8_t next_[3];
    int8_t col_ = 0;
    int8_t row_ = 0; // linha da celula de baixo; negativa: ainda acima do poco
    uint8_t fallTicks_ = 0;
    uint8_t aHold_ = 0;
    uint8_t bHold_ = 0;
    uint8_t flash_ = 0; // ticks restantes de pisca; 0 com a peca caindo
    uint8_t chain_ = 0;
    uint16_t removed_ = 0;
    uint16_t score_ = 0;
    bool over_ = false;
    uint32_t rng_ = 1;
};

} // namespace twobtn
} // namespace game
} // namespace mrm
