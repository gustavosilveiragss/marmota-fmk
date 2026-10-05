#pragma once

#include "Mp3Probe.h"

namespace mrm {

constexpr size_t kMaxArtistLen = 48; // bytes UTF-8, sem o NUL

// Artista do MP3 (ID3v2.3/2.4 TPE1, não a 2.2; sem ele, ID3v1) como UTF-8 pronto para o OLED: só Latin-1, o que
// não cabe vira '?' e aspas e travessões tipográficos viram ASCII. out tem kMaxArtistLen + 1 bytes e
// fica vazio quando não há artista. Puro (sem Arduino) e limitado: lê só cabeçalhos de frame e até
// 128 bytes do texto, então capa embutida ou tag corrompida não pesam nem estouram buffer.
bool readMp3Artist(Mp3ReadAt readAt, void* ctx, uint32_t fileSize, char* out);

} // namespace mrm
