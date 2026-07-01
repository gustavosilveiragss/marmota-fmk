#include "WifiPortal.h"

#include <WiFi.h>
#include <LittleFS.h>

namespace mrm {

namespace {
constexpr byte kDnsPort = 53;
constexpr const char* kUploadRoute = "/upload";
} // namespace

void WifiPortal::begin() {
    tmpPath_ = String(config_.destPath) + ".tmp";
    uploadError_ = false;
    done_ = false;

    WiFi.mode(WIFI_AP);
    WiFi.softAP(config_.ssid);
    WiFi.setTxPower(WIFI_POWER_8_5dBm);

    server_.on("/", HTTP_GET, [this] { sendPage(); });
    server_.on(kUploadRoute, HTTP_POST, [this] { server_.send(uploadError_ ? 500 : 200, "text/plain", uploadError_ ? "fail" : "ok"); }, [this] { handleUpload(); });
    if (routes_)
        routes_(server_);
    server_.onNotFound([this] { sendRedirect(); });
    server_.begin();

    dns_.start(kDnsPort, "*", WiFi.softAPIP());
}

void WifiPortal::handle() {
    dns_.processNextRequest();
    server_.handleClient();
}

void WifiPortal::end() {
    dns_.stop();
    server_.stop();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);
}

void WifiPortal::handleUpload() {
    HTTPUpload& up = server_.upload();
    if (up.status == UPLOAD_FILE_START) {
        uploadError_ = false;
        upload_ = LittleFS.open(tmpPath_, "w");
        if (!upload_)
            uploadError_ = true;
    } else if (up.status == UPLOAD_FILE_WRITE) {
        if (upload_ && !uploadError_ && upload_.write(up.buf, up.currentSize) != up.currentSize)
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
        LittleFS.remove(config_.destPath);
        if (LittleFS.rename(tmpPath_, config_.destPath))
            done_ = true;
        else
            uploadError_ = true;
    }
}

void WifiPortal::sendPage() {
    if (config_.page)
        server_.send(200, "text/html", config_.page);
    else
        server_.send(200, "text/plain", "marmota");
}

void WifiPortal::sendRedirect() {
    server_.sendHeader("Location", "http://192.168.4.1/", true);
    server_.send(302, "text/plain", "");
}

} // namespace mrm
