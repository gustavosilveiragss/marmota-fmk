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
    // Busca: tabela do Xing (VBR) e a regiao que ela cobre; sem tabela vale a taxa media (exata em CBR).
    bool hasToc = false;
    uint8_t toc[100]{};      // posicao no arquivo (x/256) a cada 1% do tempo
    uint32_t firstFrame = 0; // inicio do audio, no frame Xing quando existe
    uint32_t audioBytes = 0; // do firstFrame ate o fim do audio
};

// Le ate n bytes a partir de offset; devolve quantos leu.
using Mp3ReadAt = size_t (*)(void* ctx, uint32_t offset, uint8_t* buf, size_t n);

// Pula as tags ID3v2 e exige um frame MPEG-1/2/2.5 Layer III confirmado pelo frame seguinte (ou que
// termine o arquivo). false se nao achar nos primeiros 64 KB depois da tag.
bool probeMp3(Mp3ReadAt readAt, void* ctx, uint32_t fileSize, Mp3Info& out);

// Byte do arquivo onde tocar o instante ms (limitado a durationMs): decodificar dali e achar o proximo
// sync sai no instante certo, dentro de um frame. Interpola a tabela do Xing quando ela existe.
uint32_t mp3OffsetAt(const Mp3Info& info, uint32_t ms);

} // namespace mrm
