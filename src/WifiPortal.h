#pragma once

#include <Arduino.h>
#include <FS.h>
#include <WebServer.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <functional>

namespace mrm {

// Socket do cliente em atendimento. WebServer::client() devolve cópia, então setTimeout/stop por ela não
// chegam ao socket; o membro protegido _currentClient e alcançado sem alterar a lib.
WiFiClient& currentClient(WebServer& server);

// Espera por dados do corpo: o Stream recebe ms, e passar segundos dava 5 ms e abortava na primeira pausa.
void setRecvTimeout(WebServer& server, uint32_t seconds);

class WifiPortal {
public:
    struct Config {
        const char* ssid = "marmota";
        const char* destPath = "/upload.bin"; // onde um upload válido e guardado, nullptr desliga /upload
        const char* page = nullptr;           // HTML servido em GET /
        const uint8_t* pageGz = nullptr;      // página gzip em PROGMEM, tem prioridade sobre page
        size_t pageGzLen = 0;
        uint8_t channel = 1;
        uint8_t maxClients = 4;
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
    void sendUploadResult();
    void sendPage();
    void sendRedirect();
    void addRoutes();

    Config config_;
    Validator validate_;
    RouteHook routes_;
    WebServer server_{80};
    DNSServer dns_;
    File upload_;
    String tmpPath_;
    bool badRequest_ = false;
    bool uploadError_ = false;
    bool done_ = false;
    bool routed_ = false; // o WebServer guarda as rotas entre end e begin
};

} // namespace mrm
