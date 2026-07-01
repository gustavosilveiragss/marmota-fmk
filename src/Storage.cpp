#include "Storage.h"

#include <LittleFS.h>

namespace mrm {

bool fsMount(bool formatOnFail) {
    return LittleFS.begin(formatOnFail);
}

bool Config::load() {
    File file = LittleFS.open(path_, "r");
    if (!file)
        return false;
    DeserializationError err = deserializeJson(doc_, file);
    file.close();
    return !err;
}

bool Config::save() const {
    File file = LittleFS.open(path_, "w");
    if (!file)
        return false;
    const bool ok = serializeJson(doc_, file) > 0;
    file.close();
    return ok;
}

} // namespace mrm
