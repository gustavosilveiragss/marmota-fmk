#pragma once

#include <Arduino.h>

#ifndef MRM_LOG
#define MRM_LOG 1
#endif

namespace mrm {

inline void logBegin(unsigned long baud = 115200) {
#if MRM_LOG
    Serial.begin(baud);
#else
    (void)baud;
#endif
}

template <typename... Args>
inline void log(Args... args) {
#if MRM_LOG
    (Serial.print(args), ...);
    Serial.println();
#else
    (void)sizeof...(args);
#endif
}

} // namespace mrm
