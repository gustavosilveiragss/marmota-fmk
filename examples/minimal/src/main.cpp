#include <marmota.h>
#include <LittleFS.h>

using namespace mrm;

namespace {

constexpr uint8_t kButtonPin = 9;
constexpr uint8_t kLedPin = 8;

const char kPage[] = "<!doctype html><meta charset=utf-8><h1>marmota</h1>"
                     "<form method=post action=/upload enctype=multipart/form-data>"
                     "<input type=file name=f><button>send</button></form>";

const char* const kLines[] = {"pagina abre sozinha", "ou va em 192.168.4.1"};

Battery battery;
Button button(kButtonPin);
Ssd1306Display display;
Led led({.pin = kLedPin});
WifiPortal portal({.ssid = "marmota", .destPath = "/show.bin", .page = kPage});

void runPortal() {
    StatusScreen screen(display);
    display.reinit();
    portal.begin();
    display.reinit();
    uint8_t step = 0;
    while (!portal.done()) {
        portal.handle();
        battery.update();
        const StatusScreen::Config status{
            .title = "marmota",
            .lines = kLines,
            .lineCount = sizeof(kLines) / sizeof(kLines[0]),
            .battery = battery.percent(),
            .showBattery = true,
            .showWifi = true,
        };
        screen.draw(status, step++);
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
