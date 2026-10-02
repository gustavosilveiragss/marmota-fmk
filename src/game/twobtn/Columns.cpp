#include "Columns.h"

#include "Text.h"

namespace mrm {
namespace game {
namespace twobtn {

namespace {

constexpr int16_t kWellX = 36;
constexpr int16_t kWellY = 13;
constexpr int16_t kCell = 8;
constexpr int16_t kNextX = 100;
constexpr uint8_t kStartCol = 2;
constexpr uint8_t kBaseFall = 14;
constexpr uint8_t kMinFall = 8;
constexpr uint8_t kBlocksPerLevel = 30;
constexpr uint8_t kFourthFrom = 60;
constexpr uint8_t kHoldTicks = 7; // B ate aqui e toque (gira), a partir daqui derruba
constexpr uint8_t kIgnore = 255;  // B segurado de uma peca anterior
constexpr uint8_t kRepeatFrom = 10;
constexpr uint8_t kRepeatEvery = 5;
constexpr uint8_t kFlashTicks = 12;
constexpr uint8_t kFastFall = 2;
constexpr GameText kText = {txt(Str::GameColumns), txt(Str::GoalColumns), txt(Str::KeyMove), txt(Str::KeyRotate), {txt(Str::ColumnsR1), txt(Str::ColumnsR2), txt(Str::ColumnsR3)}};

// Simbolo 1 a 4 numa celula de lado size: quadrado cheio, quadrado vazado, X e bolinha.
void symbol(mrm::Panel& o, int16_t x, int16_t y, int16_t size, uint8_t kind) {
    const int16_t s = size - 2;
    switch (kind) {
    case 1:
        o.fillRect(x + 1, y + 1, s, s);
        break;
    case 2:
        o.drawRect(x + 1, y + 1, s, s);
        break;
    case 3:
        o.drawLine(x + 1, y + 1, x + s, y + s);
        o.drawLine(x + 1, y + s, x + s, y + 1);
        break;
    default:
        o.fillCircle(x + size / 2, y + size / 2, s / 2);
        break;
    }
}

} // namespace

const GameText& Columns::text() const {
    return kText;
}

uint8_t Columns::randomSymbol() {
    const uint8_t kinds = removed_ >= kFourthFrom ? 4 : 3;
    return 1 + nextRandom(rng_) % kinds;
}

void Columns::reset(uint32_t seed) {
    rng_ = seed | 1;
    memset(grid_, 0, sizeof(grid_));
    memset(mark_, 0, sizeof(mark_));
    score_ = 0;
    removed_ = 0;
    chain_ = 0;
    over_ = false;
    aHold_ = bHold_ = 0;
    for (uint8_t& s : next_)
        s = randomSymbol();
    newPiece();
}

void Columns::newPiece() {
    memcpy(piece_, next_, sizeof(piece_));
    for (uint8_t& s : next_)
        s = randomSymbol();
    col_ = kStartCol;
    row_ = -1;
    fallTicks_ = 0;
    flash_ = 0;
}

bool Columns::fits(uint8_t col) const {
    for (int8_t r = max<int8_t>(row_ - 2, 0); r <= row_; ++r) {
        if (grid_[r][col])
            return false;
    }
    return true;
}

void Columns::moveRight() {
    for (uint8_t k = 1; k < kCols; ++k) {
        const uint8_t col = (col_ + k) % kCols;
        if (fits(col)) {
            col_ = col;
            return;
        }
    }
}

void Columns::rotate() {
    const uint8_t bottom = piece_[2];
    piece_[2] = piece_[1];
    piece_[1] = piece_[0];
    piece_[0] = bottom;
}

// Toque em B gira ao soltar; segurar derruba. Depois de travar, o B ainda segurado nao age na peca nova.
void Columns::handleB(bool down) {
    if (down) {
        if (bHold_ != kIgnore && bHold_ < kHoldTicks)
            ++bHold_;
        return;
    }
    if (bHold_ != kIgnore && bHold_ > 0 && bHold_ < kHoldTicks)
        rotate();
    bHold_ = 0;
}

void Columns::tick(const GameInput& in) {
    if (flash_) {
        if (--flash_ == 0)
            clearMarked();
        return;
    }
    if (in.a) {
        aHold_ = aHold_ >= 200 ? kRepeatFrom : aHold_ + 1;
        if (in.aPress || (aHold_ >= kRepeatFrom && (aHold_ - kRepeatFrom) % kRepeatEvery == 0))
            moveRight();
    } else {
        aHold_ = 0;
    }
    handleB(in.b);
    const uint8_t level = removed_ / kBlocksPerLevel;
    const uint8_t interval = bHold_ == kHoldTicks ? kFastFall : max<int16_t>(kMinFall, kBaseFall - 2 * level);
    if (++fallTicks_ >= interval)
        fall();
}

void Columns::fall() {
    fallTicks_ = 0;
    if (row_ + 1 < kRows && !grid_[row_ + 1][col_])
        ++row_;
    else
        lock();
}

void Columns::lock() {
    bHold_ = kIgnore;
    if (row_ < 2) {
        over_ = true; // a ponta de cima ficou acima do poco
        return;
    }
    for (uint8_t i = 0; i < 3; ++i)
        grid_[row_ - 2 + i][col_] = piece_[i];
    chain_ = 0;
    resolve();
}

void Columns::markRun(int8_t r, int8_t c, int8_t dr, int8_t dc) {
    const int8_t r2 = r + 2 * dr;
    const int8_t c2 = c + 2 * dc;
    if (r2 < 0 || r2 >= kRows || c2 < 0 || c2 >= kCols)
        return;
    const uint8_t s = grid_[r][c];
    if (!s || grid_[r + dr][c + dc] != s || grid_[r2][c2] != s)
        return;
    for (int8_t k = 0; k < 3; ++k)
        mark_[r + k * dr] |= 1u << (c + k * dc);
}

uint8_t Columns::markMatches() {
    static constexpr int8_t kDirs[4][2] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};
    uint8_t count = 0;
    for (int8_t r = 0; r < kRows; ++r) {
        for (int8_t c = 0; c < kCols; ++c) {
            for (const auto& d : kDirs)
                markRun(r, c, d[0], d[1]);
        }
    }
    for (const uint8_t row : mark_)
        count += __builtin_popcount(row);
    return count;
}

void Columns::resolve() {
    if (markMatches()) {
        ++chain_;
        flash_ = kFlashTicks;
    } else {
        chain_ = 0;
        newPiece();
    }
}

void Columns::clearMarked() {
    uint8_t count = 0;
    for (uint8_t c = 0; c < kCols; ++c) {
        uint8_t write = kRows;
        for (int8_t r = kRows - 1; r >= 0; --r) {
            if (marked(r, c)) {
                ++count;
            } else if (grid_[r][c]) {
                grid_[--write][c] = grid_[r][c];
            }
        }
        while (write > 0)
            grid_[--write][c] = 0;
    }
    memset(mark_, 0, sizeof(mark_));
    score_ += count * chain_;
    removed_ += count;
    resolve();
}

void Columns::draw(mrm::Panel& o, uint32_t now) const {
    (void)now;
    hud(o, score_, 0);
    o.drawRect(kWellX - 1, kFieldTop, kCols * kCell + 2, kRows * kCell + 2);
    const bool blink = flash_ && (flash_ / 2) % 2;
    for (uint8_t r = 0; r < kRows; ++r) {
        for (uint8_t c = 0; c < kCols; ++c) {
            if (grid_[r][c] && !(blink && marked(r, c)))
                symbol(o, kWellX + c * kCell, kWellY + r * kCell, kCell, grid_[r][c]);
        }
    }
    if (!flash_) {
        for (int8_t i = 0; i < 3; ++i) {
            if (row_ - 2 + i >= 0)
                symbol(o, kWellX + col_ * kCell, kWellY + (row_ - 2 + i) * kCell, kCell, piece_[i]);
        }
    }
    for (uint8_t i = 0; i < 3; ++i)
        symbol(o, kNextX, kWellY + 2 + i * kCell, kCell, next_[i]);
    if (chain_ >= 2) {
        char text[6];
        snprintf(text, sizeof(text), "x%u", chain_);
        o.setFont(ArialMT_Plain_10);
        o.setTextAlignment(TEXT_ALIGN_CENTER);
        o.drawText(kNextX + 4, 46, text);
    }
}

// B gira a peca, A a leva duas colunas para a direita, ela cai e tres iguais piscam e somem.
uint8_t Columns::demo(mrm::Panel& o, uint32_t now) const {
    constexpr uint32_t kLoop = 3600;
    constexpr int16_t kSize = 6;
    constexpr int16_t kX = 46;
    const uint32_t t = now % kLoop;
    const int16_t floor = 48;
    o.drawHorizontalLine(kX - 1, floor, kCols * kSize + 2);
    const bool landed = t >= 1900;
    const bool gone = t >= 2500;
    const bool blink = landed && !gone && (t / 100) % 2;
    if (!gone && !blink) {
        symbol(o, kX + 3 * kSize, floor - kSize, kSize, 1);
        symbol(o, kX + 4 * kSize, floor - kSize, kSize, 1);
    }
    const int8_t col = t < 850 ? 0 : (t < 1250 ? 1 : 2);
    const int16_t top = t < 1600 ? 26 : (t < 1900 ? 26 + (t - 1600) / 75 : 30);
    static constexpr uint8_t kBefore[3] = {2, 1, 3}; // vazado, cheio, X
    static constexpr uint8_t kAfter[3] = {3, 2, 1};  // X, vazado, cheio
    const uint8_t* order = t < 400 ? kBefore : kAfter;
    for (int8_t i = 0; i < 3; ++i) {
        if (i == 2 && (gone || blink))
            continue;
        symbol(o, kX + col * kSize, gone ? top + (i + 1) * kSize : top + i * kSize, kSize, order[i]);
    }
    if (t >= 300 && t < 500)
        return kLitB;
    return (t >= 800 && t < 900) || (t >= 1200 && t < 1300) ? kLitA : 0;
}

} // namespace twobtn
} // namespace game
} // namespace mrm
