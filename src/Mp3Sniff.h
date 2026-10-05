#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "Mp3Header.h"
#include "UploadVerdict.h"

namespace mrm {

// Valida MP3 em streaming, em fatias de qualquer tamanho: pula a tag ID3v2 e exige um frame
// Layer III válido seguido de outro frame com os mesmos parâmetros.
struct Mp3Sniff {
    using Result = UploadVerdict;

    void reset() { *this = Mp3Sniff{}; }

    Result feed(const uint8_t* bytes, size_t count) {
        for (size_t i = 0; i < count && result_ == Result::Need; ++i)
            step(bytes[i]);
        return result_;
    }

    uint32_t sampleRate() const { return rate_; }

private:
    enum class Phase : uint8_t { Id3,
                                 Skip,
                                 Hunt,
                                 Gap,
                                 Confirm };

    static constexpr uint32_t kSearchLimit = 4096;
    static constexpr size_t kId3HeaderBytes = 10;
    static constexpr size_t kId3SizeAt = 6;
    static constexpr uint8_t kId3FooterFlag = 0x10;
    static constexpr uint32_t kId3FooterBytes = 10;
    static constexpr uint32_t kFrameHeaderBytes = 4;

    Phase phase_ = Phase::Id3;
    Result result_ = Result::Need;
    uint8_t count_ = 0;
    uint8_t id3_[kId3HeaderBytes] = {};
    uint32_t window_ = 0;
    uint32_t first_ = 0;
    uint32_t left_ = 0;
    uint32_t searched_ = 0;
    uint32_t rate_ = 0;

    // Tamanho do frame em bytes, ou 0 se o cabeçalho não e Layer III válido.
    static uint32_t frameLen(uint32_t header) { return mp3header::isValidLayer3(header) ?
            mp3header::frameBytesOf(header) : 0; }

    void step(uint8_t byte) {
        switch (phase_) {
        case Phase::Id3:
            return id3Byte(byte);
        case Phase::Skip:
            if (--left_ == 0)
                phase_ = Phase::Hunt;
            return;
        case Phase::Hunt:
            return hunt(byte);
        case Phase::Gap:
            if (--left_ == 0) {
                phase_ = Phase::Confirm;
                count_ = 0;
            }

            return;
        case Phase::Confirm:
            return confirm(byte);
        }
    }

    void id3Byte(uint8_t byte) {
        id3_[count_++] = byte;
        if (count_ <= 3 && byte != static_cast<uint8_t>("ID3"[count_ - 1])) {
            phase_ = Phase::Hunt;
            const uint8_t seen = count_;
            for (uint8_t i = 0; i < seen; ++i)
                hunt(id3_[i]);
            return;
        }

        if (count_ < sizeof(id3_))
            return;
        uint32_t size = 0;
        for (size_t i = kId3SizeAt; i < kId3HeaderBytes; ++i) {
            if (id3_[i] & 0x80) {
                result_ = Result::Bad;
                return;
            }

            size = (size << 7) | id3_[i];
        }

        left_ = size + ((id3_[5] & kId3FooterFlag) ? kId3FooterBytes : 0);
        phase_ = left_ ? Phase::Skip : Phase::Hunt;
    }

    bool startFrame(uint32_t header) {
        const uint32_t len = frameLen(header);
        if (len == 0)
            return false;
        first_ = header;
        left_ = len - kFrameHeaderBytes;
        phase_ = Phase::Gap;
        return true;
    }

    void hunt(uint8_t byte) {
        window_ = (window_ << 8) | byte;
        if (startFrame(window_))
            return;

        if (++searched_ >= kSearchLimit)
            result_ = Result::Bad;
    }

    void confirm(uint8_t byte) {
        window_ = (window_ << 8) | byte;
        if (++count_ < kFrameHeaderBytes)
            return;

        if (frameLen(window_) != 0 && ((window_ ^ first_) & mp3header::kSameStreamMask) == 0) {
            rate_ = mp3header::sampleRateOf(first_);
            result_ = Result::Ok;
            return;
        }

        phase_ = Phase::Hunt;
        if (!startFrame(window_) && ++searched_ >= kSearchLimit)
            result_ = Result::Bad;
    }
};

} // namespace mrm
