#include <marmota.h>
#include <LittleFS.h>

using namespace mrm;

namespace {

constexpr uint8_t kButtonPin = 9;
constexpr uint8_t kLedPin = 8;

const char kPage[] = "<!doctype html><meta charset=utf-8><h1>marmota</h1>"
                     "<form method=post action=/upload enctype=multipart/form-data>"
                     "<input type=file name=f><button>send</button></form>";

Battery battery;
Button button(kButtonPin);
Ssd1306Display display;
Led led({.pin = kLedPin});
WifiPortal portal({.ssid = "marmota", .destPath = "/show.bin", .page = kPage});

void runPortal() {
    display.reinit();
    portal.begin();
    display.reinit();
    while (!portal.done()) {
        portal.handle();
        battery.update();
        display.clear();
        display.line(0, "wifi upload");
        display.batteryBadge(battery.percent());
        display.line(24, "192.168.4.1");
        display.show();
        if (button.poll() == Gesture::Single)
            break;
        delay(10);
    }
    portal.end();
    display.reinit();
}

} // namespace

void setup() {
    logBegin();
    fsMount();
    battery.begin();
    button.begin();
    display.begin();
    led.begin();
    portal.onValidate([](const char* path) { return LittleFS.exists(path); });
}

void loop() {
    battery.update();
    led.heartbeat();
    if (button.poll() == Gesture::Double)
        runPortal();
    delay(10);
}
