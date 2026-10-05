#include "CrashLog.h"

#include <esp_attr.h>
#include <esp_system.h>
#include <stdarg.h>

#include "LogRing.h"

namespace mrm::crashlog {
namespace {

RTC_NOINIT_ATTR LogRing<kLines, kWidth> ring;
portMUX_TYPE lock = portMUX_INITIALIZER_UNLOCKED;

const char* resetName(esp_reset_reason_t reason) {
    switch (reason) {
        case ESP_RST_POWERON: return "power";
        case ESP_RST_EXT: return "ext";
        case ESP_RST_SW: return "sw";
        case ESP_RST_PANIC: return "panic";
        case ESP_RST_INT_WDT: return "int_wdt";
        case ESP_RST_TASK_WDT: return "task_wdt";
        case ESP_RST_WDT: return "wdt";
        case ESP_RST_DEEPSLEEP: return "deepsleep";
        case ESP_RST_BROWNOUT: return "brownout";
        default: return "outro";
    }
}

} // namespace

void begin() {
    const esp_reset_reason_t reason = esp_reset_reason();
    const bool kept = ring.recover();

    if (reason == ESP_RST_POWERON || !kept)
        ring.clear();

    note("boot reset=%s heap=%u", resetName(reason), static_cast<unsigned>(ESP.getFreeHeap()));
}

void note(const char* format, ...) {
    char text[kWidth + 16];
    const unsigned long ms = millis();
    const int stamp = snprintf(text, sizeof(text), "%lu.%lu ", ms / 1000, (ms / 100) % 10);

    va_list args;
    va_start(args, format);
    vsnprintf(text + stamp, sizeof(text) - stamp, format, args);
    va_end(args);

    portENTER_CRITICAL(&lock);
    ring.push(text);
    portEXIT_CRITICAL(&lock);
}

uint8_t count() {
    return ring.count;
}

bool copy(uint8_t index, char* out, size_t size) {
    if (size == 0)
        return false;

    portENTER_CRITICAL(&lock);
    const bool exists = index < ring.count;
    if (exists)
        strlcpy(out, ring.at(index), size);

    portEXIT_CRITICAL(&lock);

    return exists;
}

void dump(Print& out) {
    char line[kWidth];

    for (uint8_t i = 0; copy(i, line, sizeof(line)); ++i)
        out.println(line);
}

} // namespace mrm::crashlog
