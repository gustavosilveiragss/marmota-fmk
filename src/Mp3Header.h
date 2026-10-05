#pragma once

#include <cstdint>

namespace mrm {

// Campos do cabeçalho de 4 bytes de um frame MPEG áudio (lido como inteiro big-endian).
namespace mp3header {

constexpr uint32_t kSyncBits = 11;
constexpr uint32_t kSyncMask = 0x7FF;
constexpr uint32_t kVersionMpeg1 = 3;
constexpr uint32_t kVersionMpeg2 = 2;
constexpr uint32_t kVersionReserved = 1; // 0 = MPEG-2.5
constexpr uint32_t kLayer3 = 1;
constexpr uint32_t kBitrateFree = 0;
constexpr uint32_t kBitrateBad = 15;
constexpr uint32_t kRateReserved = 3;
constexpr uint32_t kChannelModeMono = 3;
constexpr uint32_t kBytesPerSlotMpeg1 = 144; // 1152 amostras / 8
constexpr uint32_t kBytesPerSlotMpeg2 = 72;  // 576 amostras / 8
constexpr uint16_t kSamplesMpeg1 = 1152;
constexpr uint16_t kSamplesMpeg2 = 576;
constexpr uint32_t kSameStreamMask = 0xFFFE0C00; // sync, versão, camada e taxa de amostragem
constexpr uint16_t kBitrateMpeg1Kbps[16] = {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 0};
constexpr uint16_t kBitrateMpeg2Kbps[16] = {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0};
constexpr uint16_t kRateMpeg1Hz[3] = {44100, 48000, 32000};

constexpr uint32_t syncOf(uint32_t header) {
    return header >> (32 - kSyncBits);
}
constexpr uint32_t versionOf(uint32_t header) {
    return (header >> 19) & 3;
}
constexpr uint32_t layerOf(uint32_t header) {
    return (header >> 17) & 3;
}
constexpr uint32_t bitrateIndexOf(uint32_t header) {
    return (header >> 12) & 15;
}
constexpr uint32_t rateIndexOf(uint32_t header) {
    return (header >> 10) & 3;
}
constexpr uint32_t paddingOf(uint32_t header) {
    return (header >> 9) & 1;
}
constexpr uint32_t channelModeOf(uint32_t header) {
    return (header >> 6) & 3;
}

constexpr bool isMpeg1(uint32_t header) {
    return versionOf(header) == kVersionMpeg1;
}

// Layer III com versão, taxa e bitrate válidos (o bitrate "livre" não é suportado).
constexpr bool isValidLayer3(uint32_t header) {
    return syncOf(header) == kSyncMask && versionOf(header) != kVersionReserved && layerOf(header) == kLayer3 &&
           bitrateIndexOf(header) != kBitrateFree &&
           bitrateIndexOf(header) != kBitrateBad && rateIndexOf(header) != kRateReserved;
}

// Sample rate: MPEG-2 e MPEG-2.5 dividem a tabela do MPEG-1 por 2 e 4.
constexpr uint32_t sampleRateOf(uint32_t header) {
    const uint32_t base = kRateMpeg1Hz[rateIndexOf(header)];
    return isMpeg1(header) ? base : (versionOf(header) == kVersionMpeg2 ? base / 2 : base / 4);
}

constexpr uint32_t kbpsOf(uint32_t header) {
    return isMpeg1(header) ? kBitrateMpeg1Kbps[bitrateIndexOf(header)] : kBitrateMpeg2Kbps[bitrateIndexOf(header)];
}

// Tamanho do frame em bytes; só vale para cabeçalho que passou em isValidLayer3.
constexpr uint32_t frameBytesOf(uint32_t header) {
    const uint32_t bytesPerSlot = isMpeg1(header) ? kBytesPerSlotMpeg1 : kBytesPerSlotMpeg2;
    return bytesPerSlot * kbpsOf(header) * 1000 / sampleRateOf(header) + paddingOf(header);
}

} // namespace mp3header

} // namespace mrm
