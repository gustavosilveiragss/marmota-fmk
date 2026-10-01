#include "Identity.h"

namespace mrm {

namespace {
constexpr char kAlphabet[] = "23456789ABCDEFGHJKLMNPQRSTUVWXYZ";
constexpr uint32_t kBase = sizeof(kAlphabet) - 1;
} // namespace

void deviceCode(char out[5]) {
    const uint64_t mac = ESP.getEfuseMac();
    uint32_t hash = 2166136261u; // FNV-1a: espalha MACs de lote seguido, que so mudam nos ultimos bytes
    for (uint8_t i = 0; i < 6; ++i) {
        hash ^= static_cast<uint8_t>(mac >> (8 * i));
        hash *= 16777619u;
    }
    for (int i = 3; i >= 0; --i) {
        out[i] = kAlphabet[hash % kBase];
        hash /= kBase;
    }
    out[4] = '\0';
}

} // namespace mrm
