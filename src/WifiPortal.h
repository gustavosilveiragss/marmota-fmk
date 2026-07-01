#pragma once

#include <Arduino.h>
#include <FS.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <functional>

namespace mrm {

class WifiPortal {
public:
    struct Config {
        const char* ssid = "marmota";
        const char* destPath = "/upload.bin"; // where a valid upload is stored
        const char* page = nullptr;           // HTML served at GET /
    };

    using Validator = std::function<bool(const char* tmpPath)>;
    using RouteHook = std::function<void(WebServer&)>;

    explicit WifiPortal(const Config& config)
        : config_(config) {}

    void onValidate(Validator validator) { validate_ = std::move(validator); }
    void onRoutes(RouteHook hook) { routes_ = std::move(hook); }

    void begin();
    void handle();
    void end();
    bool done() const { return done_; }

private:
    void handleUpload();
    void sendPage();
    void sendRedirect();

    Config config_;
    Validator validate_;
    RouteHook routes_;
    WebServer server_{80};
    DNSServer dns_;
    File upload_;
    String tmpPath_;
    bool uploadError_ = false;
    bool done_ = false;
};

} // namespace mrm
