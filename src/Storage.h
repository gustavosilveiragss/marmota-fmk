#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

namespace mrm {

bool fsMount(bool formatOnFail = true);

class Config {
public:
    explicit Config(const char* path = "/config.json")
        : path_(path) {}

    bool load();
    bool save() const;

    JsonDocument& doc() { return doc_; }
    const JsonDocument& doc() const { return doc_; }

private:
    const char* path_;
    JsonDocument doc_;
};

} // namespace mrm
