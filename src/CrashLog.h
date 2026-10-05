#pragma once

#include <Arduino.h>

namespace mrm::crashlog {

constexpr uint8_t kLines = 32;
constexpr uint8_t kWidth = 56;

// Anel de eventos na memória RTC. Sobrevive a reset de software, WDT, panic e brownout. Perde-se ao desligar a energia.
// Chame begin() no começo do setup. note() pode vir de qualquer task, nunca de ISR.
void begin();
void note(const char* format, ...) __attribute__((format(printf, 1, 2)));

uint8_t count();
// 0 é a mais antiga. Se entrar evento no meio de uma listagem, uma linha pode sair repetida ou faltar.
bool copy(uint8_t index, char* out, size_t size);
void dump(Print& out);

} // namespace mrm::crashlog
