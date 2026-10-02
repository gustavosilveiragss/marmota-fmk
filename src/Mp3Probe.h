#pragma once

#include <cstddef>
#include <cstdint>

namespace mrm {

// O que o player precisa saber de um MP3 antes de tocar. Puro (sem Arduino): o arquivo chega por
// uma funcao de leitura com offset, entao funciona com File, FILE* ou memoria.
struct Mp3Info {
    uint32_t dataOffset = 0; // primeiro frame de audio (depois do ID3v2 e do frame Xing/VBRI)
    uint32_t durationMs = 0; // pelo Xing/VBRI; sem eles, tamanho x 8 / bitrate
    uint32_t sampleRate = 0;
    uint16_t kbps = 0; // do primeiro frame
    uint8_t channels = 0;
};

// Le ate n bytes a partir de offset; devolve quantos leu.
using Mp3ReadAt = size_t (*)(void* ctx, uint32_t offset, uint8_t* buf, size_t n);

// Pula as tags ID3v2 e exige um frame MPEG-1/2/2.5 Layer III confirmado pelo frame seguinte (ou que
// termine o arquivo). false se nao achar nos primeiros 64 KB depois da tag.
bool probeMp3(Mp3ReadAt readAt, void* ctx, uint32_t fileSize, Mp3Info& out);

} // namespace mrm
