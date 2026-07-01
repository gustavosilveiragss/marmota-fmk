# marmota-fmk

Shared ESP32 firmware framework for the [marmota](https://github.com/gustavosilveiragss/marmota)
device line. It holds the parts every device repeats so each project keeps only its
own logic: battery reading, an on-demand WiFi upload portal, button gestures, an OLED
wrapper, power and storage.

## Modules

All under `namespace mrm`.

| Module | Type | Does |
|---|---|---|
| `Battery` | `Battery.h/.cpp` | LiPo voltage and percent over an ADC divider, gated reads |
| `Button` | `Button.h` | debounced single, double and long press gestures |
| `Ssd1306Display` | `Ssd1306Display.h/.cpp` | OLED init, reinit after radio toggles, text and battery widgets |
| `WifiPortal` | `WifiPortal.h/.cpp` | on-demand AP, captive DNS, file upload with validation, radio off |
| `Power` | `Power.h/.cpp` | status LED heartbeat, radio off, light and deep sleep |
| `Storage` | `Storage.h/.cpp` | LittleFS mount and a JSON config file |
| `Log` | `Log.h` | thin serial log, compiles out with `-DMRM_LOG=0` |

The portal is networking only: the project supplies the HTML page, an upload
validator and any extra routes, and owns its own display and loop.

## Use it

`platformio.ini`:

```ini
[env]
platform = espressif32
framework = arduino
build_flags = -std=gnu++2a
build_unflags = -std=gnu++11
lib_deps = https://github.com/gustavosilveiragss/marmota-fmk.git#v0.1.0
```

```cpp
#include <marmota.h>
using namespace mrm;

Battery battery;
Button button(9);
```

Boards in the line: ESP32-C3 (`lolin_c3_mini`) for pin, paperback and stomp; classic
ESP32 (`esp32dev`) for mp3 (needs Bluetooth A2DP); ESP32-CAM (`esp32cam`) for cam.

## Editor

clangd config ships in `.clangd`. Generate the compile database once with
`pio run -t compiledb`, then clangd resolves the Arduino and framework headers.
Format with the bundled `.clang-format`.

## License

MIT.
