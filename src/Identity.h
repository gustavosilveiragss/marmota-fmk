#pragma once

#include <Arduino.h>

namespace mrm {

// Codigo de 4 caracteres (A-Z e 2-9, sem 0, 1, I e O que se confundem no display) fixo por chip, derivado
// do MAC de fabrica. Serve de sufixo no nome do device no Wi-Fi e no Bluetooth ("marmota mp3 K7QX").
void deviceCode(char out[5]);

} // namespace mrm
