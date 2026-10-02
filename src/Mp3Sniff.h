#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace mrm {

// Valida MP3 em streaming, em fatias de qualquer tamanho: pula a tag ID3v2 e exige um frame
// Layer III valido seguido de outro frame com os mesmos parametros.
struct Mp3Sniff {
    enum class Result : uint8_t { Need, Ok, Bad };

    void reset() { *this = Mp3Sniff{}; }

    Result feed(const uint8_t* p, size_t n) {
        for (size_t i = 0; i < n && result_ == Result::Need; ++i)
            step(p[i]);
        return result_;
    }

    uint32_t sampleRate() const { return rate_; }

private:
    enum class Phase : uint8_t { Id3, Skip, Hunt, Gap, Confirm };

    static constexpr uint32_t kSearchLimit = 4096;
    static constexpr uint32_t kSameStream = 0xFFFE0C00; // sync, versao, camada e taxa
    static constexpr uint16_t kBitrateV1[16] = {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 0};
    static constexpr uint16_t kBitrateV2[16] = {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0};
    static constexpr uint16_t kRateV1[3] = {44100, 48000, 32000};

    Phase phase_ = Phase::Id3;
    Result result_ = Result::Need;
    uint8_t count_ = 0;
    uint8_t id3_[10] = {};
    uint32_t window_ = 0;
    uint32_t first_ = 0;
    uint32_t left_ = 0;
    uint32_t searched_ = 0;
    uint32_t rate_ = 0;

    static bool isV1(uint32_t h) { return ((h >> 19) & 3) == 3; }

    static uint32_t rateOf(uint32_t h) {
        const uint32_t base = kRateV1[(h >> 10) & 3];
        return isV1(h) ? base : (((h >> 19) & 3) == 2 ? base / 2 : base / 4);
    }

    // Tamanho do frame em bytes, ou 0 se o cabecalho nao e Layer III valido.
    static uint32_t frameLen(uint32_t h) {
        const uint32_t version = (h >> 19) & 3;
        const uint32_t bitrateIdx = (h >> 12) & 15;
        if ((h >> 21) != 0x7FF || version == 1 || ((h >> 17) & 3) != 1)
            return 0;
        if (bitrateIdx == 0 || bitrateIdx == 15 || ((h >> 10) & 3) == 3)
            return 0;
        const uint32_t kbps = isV1(h) ? kBitrateV1[bitrateIdx] : kBitrateV2[bitrateIdx];
        const uint32_t samples = isV1(h) ? 144 : 72;
        return samples * kbps * 1000 / rateOf(h) + ((h >> 9) & 1);
    }

    void step(uint8_t b) {
        switch (phase_) {
        case Phase::Id3:
            return id3Byte(b);
        case Phase::Skip:
            if (--left_ == 0)
                phase_ = Phase::Hunt;
            return;
        case Phase::Hunt:
            return hunt(b);
        case Phase::Gap:
            if (--left_ == 0) {
                phase_ = Phase::Confirm;
                count_ = 0;
            }
            return;
        case Phase::Confirm:
            return confirm(b);
        }
    }

    void id3Byte(uint8_t b) {
        id3_[count_++] = b;
        if (count_ <= 3 && b != static_cast<uint8_t>("ID3"[count_ - 1])) {
            phase_ = Phase::Hunt;
            const uint8_t seen = count_;
            for (uint8_t i = 0; i < seen; ++i)
                hunt(id3_[i]);
            return;
        }
        if (count_ < sizeof(id3_))
            return;
        uint32_t size = 0;
        for (int i = 6; i < 10; ++i) {
            if (id3_[i] & 0x80) {
                result_ = Result::Bad;
                return;
            }
            size = (size << 7) | id3_[i];
        }
        left_ = size + ((id3_[5] & 0x10) ? 10 : 0);
        phase_ = left_ ? Phase::Skip : Phase::Hunt;
    }

    bool startFrame(uint32_t header) {
        const uint32_t len = frameLen(header);
        if (len == 0)
            return false;
        first_ = header;
        left_ = len - 4;
        phase_ = Phase::Gap;
        return true;
    }

    void hunt(uint8_t b) {
        window_ = (window_ << 8) | b;
        if (startFrame(window_))
            return;
        if (++searched_ >= kSearchLimit)
            result_ = Result::Bad;
    }

    void confirm(uint8_t b) {
        window_ = (window_ << 8) | b;
        if (++count_ < 4)
            return;
        if (frameLen(window_) != 0 && ((window_ ^ first_) & kSameStream) == 0) {
            rate_ = rateOf(first_);
            result_ = Result::Ok;
            return;
        }
        phase_ = Phase::Hunt;
        if (!startFrame(window_) && ++searched_ >= kSearchLimit)
            result_ = Result::Bad;
    }
};

} // namespace mrm
