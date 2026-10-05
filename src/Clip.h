#pragma once

#include "Icon.h"

namespace mrm {

struct Frame {
    const gfx::Icon* icon;
    int8_t dx;
    int8_t dy;
    uint16_t durationMs;
};

struct Clip {
    const Frame* frames;
    uint8_t count;
};

constexpr uint32_t clipDurationMs(const Clip& clip) {
    uint32_t total = 0;
    for (uint8_t i = 0; i < clip.count; ++i)
        total += clip.frames[i].durationMs;
    return total;
}

// Ciclo: depois do último quadro volta ao primeiro.
constexpr const Frame& frameAt(const Clip& clip, uint32_t elapsedMs) {
    uint32_t left = elapsedMs % clipDurationMs(clip);
    for (uint8_t i = 0; i < clip.count; ++i) {
        if (left < clip.frames[i].durationMs)
            return clip.frames[i];
        left -= clip.frames[i].durationMs;
    }

    return clip.frames[clip.count - 1];
}

} // namespace mrm
