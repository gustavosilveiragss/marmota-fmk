#pragma once

#include <cstdint>

namespace mrm {

// Resultado de validar o conteúdo de um envio em fatias: precisa de mais bytes, aceito ou recusado.
enum class UploadVerdict : uint8_t { Need, Ok, Bad };

} // namespace mrm
