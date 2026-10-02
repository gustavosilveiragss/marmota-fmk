#include "WifiPortal.h"

#include <WiFi.h>
#include <LittleFS.h>

namespace mrm {

namespace {
constexpr byte kDnsPort = 53;
constexpr const char* kUploadRoute = "/upload";

// WebServer::stop() so fecha o socket de escuta: o cliente corrente (em HC_WAIT_READ, por exemplo)
// e o HTTPRaw ficam presos, segurando socket e heap ate o proximo begin(). Os membros sao
// protected e a lib nao tem API para soltar, entao o ponteiro de membro em instanciacao explicita
// os alcanca sem alterar a lib. Se o WebServer mudar esses nomes o build quebra, nao o runtime.
template <typename Tag>
struct Reach {
    using Type = typename Tag::Type;
    static inline Type member = nullptr;
};
template <typename Tag, typename Tag::Type M>
struct Grab {
    static inline const bool done = (Reach<Tag>::member = M, true);
};
struct ClientTag { using Type = WiFiClient WebServer::*; };
struct RawTag { using Type = std::unique_ptr<HTTPRaw> WebServer::*; };
struct StatusTag { using Type = HTTPClientStatus WebServer::*; };
template struct Grab<ClientTag, &WebServer::_currentClient>;
template struct Grab<RawTag, &WebServer::_currentRaw>;
template struct Grab<StatusTag, &WebServer::_currentStatus>;

void dropCurrentClient(WebServer& server) {
    (server.*Reach<ClientTag>::member).stop();
    server.*Reach<ClientTag>::member = WiFiClient();
    (server.*Reach<RawTag>::member).reset();
    server.*Reach<StatusTag>::member = HC_NONE;
}
} // namespace

void WifiPortal::begin() {
    if (config_.destPath)
        tmpPath_ = String(config_.destPath) + ".tmp";
    uploadError_ = false;
    badRequest_ = false;
    done_ = false;

    WiFi.mode(WIFI_AP);
    WiFi.softAP(config_.ssid, nullptr, config_.channel, 0, config_.maxClients);
    WiFi.setTxPower(WIFI_POWER_8_5dBm);

    if (!routed_)
        addRoutes();
    server_.begin();

    dns_.start(kDnsPort, "*", WiFi.softAPIP());
}

void WifiPortal::addRoutes() {
    routed_ = true;
    server_.on("/", HTTP_GET, [this] { sendPage(); });
    if (config_.destPath) {
        // handleUpload precisa do Content-Type para recusar POST que nao e multipart. O hook pode
        // sobrescrever a lista, entao mantenha Content-Type nela.
        static const char* kHeaders[] = {"Content-Type"};
        server_.collectHeaders(kHeaders, 1);
        server_.on(kUploadRoute, HTTP_POST, [this] { sendUploadResult(); }, [this] { handleUpload(); });
    }
    if (routes_)
        routes_(server_);
    server_.onNotFound([this] { sendRedirect(); });
}

void WifiPortal::handle() {
    dns_.processNextRequest();
    server_.handleClient();
}

void WifiPortal::end() {
    dns_.stop();
    dropCurrentClient(server_);
    server_.stop();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);
}

void WifiPortal::sendUploadResult() {
    if (badRequest_) {
        badRequest_ = false;
        server_.send(400, "text/plain", "bad request");
        return;
    }
    server_.send(uploadError_ ? 500 : 200, "text/plain", uploadError_ ? "fail" : "ok");
}

void WifiPortal::handleUpload() {
    // POST nao multipart cai no caminho raw do WebServer, onde upload() desreferencia ponteiro nulo.
    if (!server_.header("Content-Type").startsWith("multipart/form-data")) {
        badRequest_ = true;
        return;
    }
    HTTPUpload& up = server_.upload();
    if (up.status == UPLOAD_FILE_START) {
        uploadError_ = false;
        done_ = false;
        upload_ = LittleFS.open(tmpPath_, "w");
        if (!upload_)
            uploadError_ = true;
    } else if (up.status == UPLOAD_FILE_WRITE) {
        if (upload_ && !uploadError_ && upload_.write(up.buf, up.currentSize) != up.currentSize)
            uploadError_ = true;
    } else if (up.status == UPLOAD_FILE_ABORTED) {
        if (upload_)
            upload_.close();
        LittleFS.remove(tmpPath_);
        uploadError_ = true;
    } else if (up.status == UPLOAD_FILE_END) {
        if (upload_)
            upload_.close();
        if (uploadError_) {
            LittleFS.remove(tmpPath_);
            return;
        }
        const bool valid = !validate_ || validate_(tmpPath_.c_str());
        if (!valid) {
            LittleFS.remove(tmpPath_);
            uploadError_ = true;
            return;
        }
        // rename atomico por cima do destino. Um remove antes abriria uma janela onde uma falha
        // (queda de energia, flash cheio) apaga o conteudo atual sem por o novo no lugar.
        if (LittleFS.rename(tmpPath_, config_.destPath))
            done_ = true;
        else
            uploadError_ = true;
    }
}

void WifiPortal::sendPage() {
    // Host de fora (ex.: neverssl.com na sonda do portal cativo): a pagina chamaria a API com esse
    // Host e levaria recusa, entao o cliente volta para o endereco do aparelho.
    if (server_.hostHeader() != WiFi.softAPIP().toString())
        return sendRedirect();
    if (config_.pageGz && config_.pageGzLen) {
        server_.sendHeader("Content-Encoding", "gzip");
        server_.send_P(200, "text/html", reinterpret_cast<PGM_P>(config_.pageGz), config_.pageGzLen);
    } else if (config_.page)
        server_.send(200, "text/html", config_.page);
    else
        server_.send(200, "text/plain", "marmota");
}

void WifiPortal::sendRedirect() {
    server_.sendHeader("Location", "http://" + WiFi.softAPIP().toString() + "/", true);
    server_.send(302, "text/plain", "");
}

} // namespace mrm
