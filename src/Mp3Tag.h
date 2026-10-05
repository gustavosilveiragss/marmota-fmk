#pragma once

#include "Mp3Probe.h"

namespace mrm {

constexpr size_t kMaxArtistLen = 48; // bytes UTF-8, sem o NUL

// Artista do MP3 (ID3v2.3/2.4 TPE1, nao a 2.2; sem ele, ID3v1) como UTF-8 pronto para o OLED: so Latin-1, o que
// nao cabe vira '?' e aspas e travessoes tipograficos viram ASCII. out tem kMaxArtistLen + 1 bytes e
// fica vazio quando nao ha artista. Puro (sem Arduino) e limitado: le so cabecalhos de frame e ate
// 128 bytes do texto, entao capa embutida ou tag corrompida nao pesam nem estouram buffer.
bool readMp3Artist(Mp3ReadAt readAt, void* ctx, uint32_t fileSize, char* out);

} // namespace mrm
