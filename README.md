# marmota-fmk

Shared ESP32 firmware guts for the [marmota](https://github.com/gustavosilveiragss/marmota)
line: battery, on-demand WiFi upload portal, button gestures, OLED, power saving,
storage. Namespace `mrm`, PlatformIO, C++20.

## Use

```ini
lib_deps = https://github.com/gustavosilveiragss/marmota-fmk.git#v0.2.0
```

```cpp
#include <marmota.h>
using namespace mrm;
```

Builds on ESP32-C3, classic ESP32 and ESP32-CAM.

## Dev

clangd config in `.clangd`. Make the compile DB once with `pio run -t compiledb`.
Format with the bundled `.clang-format`.

## License

MIT.
