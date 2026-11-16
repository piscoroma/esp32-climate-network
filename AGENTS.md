# AGENTS.md

ESP32-C3 Arduino firmware (PlatformIO): reads an SHT31 over I2C, sends one
`SensorData` packet over ESP-NOW to a hardcoded receiver MAC, shows the reading
on the 0.42" OLED when the board has one, then deep-sleeps (currently 60 s).
The receiver (`firmware/receiver`) is the ESP32-C3 gateway. It receives
`SensorData` packets and POSTs each reading to the ingestion endpoint, and it
doubles as a Wake-on-LAN proxy; the room-summary display comes next.

## Repository layout

- `firmware/sensor-node/` — ESP32-C3 sensor nodes (this document's project).
- `firmware/receiver/` — ESP32-C3 receiver/gateway.
- `shared/sensor_data.h` — single source of truth for the ESP-NOW wire format;
  both projects include it via `-I../../shared`.

Unless a path starts with `firmware/` or `shared/`, it is relative to
`firmware/sensor-node/`.

## Commands

`pio` is not on `PATH` in this environment; use the PlatformIO venv binary.
`default_envs = oled042`, so a bare `pio run` only builds that env; select the
others with `-e`. Run from the repository root with `-d firmware/sensor-node`
(or `-d firmware/receiver`).

```bash
~/.platformio/penv/bin/pio run -d firmware/sensor-node -e oled042  # OLED board
~/.platformio/penv/bin/pio run -d firmware/sensor-node -e xiao-c3  # Seeed XIAO ESP32C3
~/.platformio/penv/bin/pio run -d firmware/sensor-node -e c3-zero  # Waveshare ESP32-C3-Zero
~/.platformio/penv/bin/pio run -d firmware/sensor-node -e oled042-debug  # stays awake
~/.platformio/penv/bin/pio run -d firmware/sensor-node -e oled042 -t upload \
    --upload-port /dev/ttyACM0
~/.platformio/penv/bin/pio device monitor -d firmware/sensor-node -e oled042 \
    -b 115200  # auto-reconnect
~/.platformio/penv/bin/pio run -d firmware/receiver -e c3  # ESP32-C3 gateway
```

There is no test suite: `test/` contains only the stock PlatformIO README.
Verify changes with a clean build plus the device's serial output. A build is
not proof of behavior — the logic only runs on hardware after wake-up.

## Board and pinout

| env | board | SDA / SCL | display | status LED |
|---|---|---|---|---|
| `oled042` | `esp32-c3-devkitm-1` | GPIO2 / GPIO20 | SSD1306 72x40 | GPIO8, plain blue |
| `xiao-c3` | `seeed_xiao_esp32c3` | GPIO6 / GPIO7 | none | `HAS_STATUS_LED 0` |
| `c3-zero` | `esp32-c3-devkitm-1` | GPIO4 / GPIO5* | none | GPIO10, WS2812 |

\* placeholder in `src/board_config.h`; set them to the actual wiring.

All three are ESP32-C3 with 4 MB flash. `board = esp32-c3-devkitm-1` only selects
a generic 4 MB C3 target; don't infer physical pins from the board name. There
is no dedicated PlatformIO board id for the Waveshare C3-Zero in platform
55.3.312.

### OLED board (purple 0.42" module)

- Left header: 5V, GND, 3V3, GPIO20 (UART RX), GPIO21 (UART TX), GPIO2 (A2),
  GPIO1 (A1), GPIO0 (A0).
- Right header: GPIO10, GPIO9 (SCL), GPIO8 (SDA), GPIO7 (SS), GPIO6 (MOSI),
  GPIO5 (A5 / MISO), GPIO4 (A4), GPIO3 (A3).
- The onboard OLED is an SSD1306 72x40 (EastRising), driven with U8g2 as
  `U8G2_SSD1306_72X40_ER_F_*`, on SDA = GPIO5 / SCL = GPIO6. GPIO5/6 double as
  SPI MISO/MOSI in the silkscreen legend.
- `src/display.*` is a generic U8g2 wrapper (`begin` / `clear` / `text` /
  `setContrast` / `powerSave`). `begin(contrastPercent = 100)` maps 0-100 to the
  U8g2 raw 0-255 range. `text()` draws and pushes the frame on every call;
  methods are no-ops until `begin()` succeeds. The font is
  `u8g2_font_7x14B_tr`; on the 72x40 panel two lines fit at y=15 and y=34.
- The ESP32-C3 has a single I2C controller (`SOC_I2C_NUM = 1`), so `Wire1` does
  not exist here. Because `Wire` is already bound to GPIO2/20 for the SHT31, a
  display on GPIO5/6 must use U8g2 software I2C:
  `U8G2_SSD1306_72X40_ER_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ 6, /* data=*/ 5, U8X8_PIN_NONE);`
  Using `..._F_HW_I2C` plus `Wire.begin(5, 6)` remaps the only bus and breaks the
  sensor.
- The SHT31 is deliberately wired to SDA = GPIO2 / SCL = GPIO20 (left header,
  A2 and RX pins), not to the labelled I2C header. Don't "fix" the constants to
  match the silkscreen.
- Consequences of that choice: GPIO20 is the default UART0 RX pin, so UART0
  cannot be used while SCL is wired there (logging runs on native USB CDC);
  GPIO2 and GPIO8/GPIO9 are ESP32-C3 strapping pins, so the I2C pull-ups must
  hold the lines high at boot.
- The red PWR LED is hardwired (VCC -> 4.7k -> LED -> GND) and cannot be turned
  off in software; it draws ~0.3 mA. The blue user LED is on GPIO8, active low.
  On battery builds, removing/raising the red LED's 4.7k resistor is the only
  way to cut that current.

### Display-less boards

- **XIAO ESP32C3**: I2C on D4 = GPIO6 (SDA), D5 = GPIO7 (SCL). Deep sleep
  ~44 uA, onboard battery charger and external-antenna connector. Deep-sleep
  GPIO wake works only on D0-D3, not on the BOOT pin.
- **Waveshare ESP32-C3-Zero**: no dedicated I2C header, so the pin pair is a
  wiring choice in `src/board_config.h`. WS2812 RGB LED on GPIO10 (about 1 mA
  quiescent even when dark) — relevant for battery use. BOOT is GPIO9; native
  USB, no UART bridge.

## Configuration

- `src/config.h` — application parameters shared by all nodes: `SLEEP_INTERVAL_US`
  (currently 60 s), `RECEIVER_MAC`, `DISPLAY_CONTRAST_PERCENT` (currently 1),
  `KEEP_DISPLAY_ON_DURING_SLEEP`, `BLINK_ON_SEND` (default true),
  `DEBUG_KEEP_AWAKE` (default 0), `BOOT_DELAY_MS` (2000 ms in debug, 500 ms
  otherwise), and `DEVICE_ID`. `DEVICE_ID` is edited per room before flashing; it
  is not an env flag.
- `src/board_config.h` — hardware per board, selected by the env's `-DBOARD_*`
  macro: `SDA_PIN`, `SCL_PIN`, `HAS_DISPLAY`. Add a new board here.
- ESP-NOW channel recovery (`config.h`): `SEND_TIMEOUT_MS`, `SEND_ATTEMPTS`,
  `SEND_RETRY_DELAY_MS`, `CHANNEL_PROBE_TIMEOUT_MS`, `SCAN_BACKOFF_CYCLES`,
  `DEFAULT_CHANNEL`, `CHANNEL_SCAN`. `lastChannel` and the backoff counter live
  in `RTC_DATA_ATTR`.
- `DISPLAY_CONTRAST_PERCENT` is passed to `display.begin()`; `Display::begin`
  defaults to 80 when called without arguments.
- `KEEP_DISPLAY_ON_DURING_SLEEP`: when false, `sleepForNextMeasurement()` calls
  `display.powerSave(true)` before sleeping, so the panel goes dark while its
  GDDRAM is retained. It exists for A/B power measurements. With true the last
  frame stays visible because the 3.3V rail survives deep sleep.

## Non-obvious constraints

- `HAS_DISPLAY` must stay a `#define` 0/1 macro, not a `constexpr bool`:
  `#if HAS_DISPLAY` silently evaluates unknown identifiers as 0, which compiles
  the display out even on the OLED board. The same applies to `DEBUG_KEEP_AWAKE`
  and to `HAS_STATUS_LED` / `STATUS_LED_RGB`.
- `StatusLed::blinkSent()` fires once per acknowledged send, gated by
  `BLINK_ON_SEND` in `config.h` (default 1), and blocks for 60 ms.
  The OLED board's LED is hardwired blue (GPIO8, active low, also a strapping
  pin): it can blink but not turn green. Only the C3-Zero has an RGB WS2812
  (GPIO10), driven with `rgbLedWrite()` from the core — no NeoPixel dependency on
  the node side.
- Debug envs (`*-debug`) set `-DDEBUG_KEEP_AWAKE=1`: `sleepForNextMeasurement()`
  returns instead of sleeping, and `loop()` repeats `measureAndSend()` every
  `SLEEP_INTERVAL_US`, so the USB CDC port stays available and full cycles keep
  running. `BOOT_DELAY_MS` becomes 2000 ms. Never leave a debug build on a
  battery-powered node.
- Display code is compiled only when `HAS_DISPLAY` is 1. U8g2 is in `lib_deps`
  for every env, but the linker drops it on display-less builds.
- `shared/sensor_data.h` is the ESP-NOW wire format (raw struct bytes, no
  serialization). It is the single source for `firmware/sensor-node` and the
  future `firmware/receiver`; both include it via `-I../../shared`. Any change
  to `SensorData` or the `DeviceId` enum affects the receiver and the web
  service. Keep fixed-width types only. A `static_assert` pins
  `sizeof(SensorData) == 20`; changing the struct fails compilation everywhere.
- IntelliSense may report that assert as `32 == 20`: the RISC-V toolchain maps
  `uint32_t` to `unsigned long`, which IntelliSense sizes with the 64-bit host
  ABI. `firmware/sensor-node/.vscode/settings.json` pins a 32-bit IntelliSense
  model. Trust the real build; don't change the expected size to silence the IDE.
- In normal builds all work happens in `setup()`; `loop()` is never reached
  because the board deep-sleeps at the end of setup. Only debug builds use
  `loop()` to repeat `measureAndSend()`. Don't move production periodic logic
  into `loop()`.
- Deep sleep wipes RAM: persist state with `RTC_DATA_ATTR` (`sequence` and
  `uptimeOffsetMs`). `uptimeOffsetMs` makes `SensorData.uptime` cumulative
  **seconds** instead of `millis()` since wake, and it must add
  `SLEEP_INTERVAL_US / 1000` at every sleep: `millis()` only measures the awake
  part of the cycle. The sleep timer runs off the internal RC oscillator, so the
  value drifts by a few percent over days — treat it as a monotonic counter, not
  as a clock. Serial must be re-initialized every wake
  (the existing `delay(500)` covers USB-CDC enumeration).
- A failed SHT31 read is retried once after 100 ms. ESP-NOW sends are retried
  `SEND_ATTEMPTS` times on the cached channel, then `sendWithChannelRecovery()`
  probes `CHANNEL_SCAN`; a successful probe updates `lastChannel`, while a full
  failure sets a scan backoff. `send()` returns `SendResult`: only `NoAck` /
  `Timeout` trigger the scan, `DriverError` does not.
- ESP-NOW uses the current WiFi channel. A receiver associated to the router
  follows the router's channel, so sensors recover by probing. The receiver must
  call `WiFi.setSleep(false)` (or it can miss ESP-NOW frames) and should log
  `WiFi.channel()`. The gateway associates at boot (it needs the network for
  ingestion), so `DEFAULT_CHANNEL = 1` on the nodes only matches straight away
  when the AP is on channel 1 — otherwise the first few cycles can be lost while
  the nodes scan.
- The receiver's Wi-Fi credentials live in `firmware/receiver/src/secrets.h`,
  which is gitignored; `secrets.example.h` is the committed template. A fresh
  clone does not build until the file is copied to `secrets.h`. The same file
  holds `AUTH_TOKEN`, the Wake-on-LAN ports and the deployment data
  (`WOL_HTTP_PORT`, `WOL_PORT`, `WOL_TARGET_MAC`, `WOL_BROADCAST`,
  `WOL_UNICAST`): none of it belongs in the repository.
- Ingestion POSTs run from `loop()`, never from the ESP-NOW receive callback:
  the callback runs in the Wi-Fi task and a blocking `HTTPClient` request there
  would stall the stack and drop frames. The callback only validates, copies the
  packet into a FreeRTOS queue (`DATA_QUEUE_DEPTH`, 8) and returns; `loop()`
  drains the queue. A single slot would silently drop every reading that arrives
  while a POST is in flight, and the copy would not be atomic between the two
  tasks.
- Duplicates are dropped in the receive callback, before the queue: the last
  `SensorData` from each sender is kept in a fixed table of 8 MACs and a packet
  identical to the stored one is counted and discarded. Compare the whole
  struct, never just `sequence` — a power cycle restarts `sequence` from 1 but
  also resets `uptime`, so an equality test on the struct cannot confuse the two.
  The table is RAM-only and does not replace a unique `(device_id, sequence)`
  key on the server.
- `HTTPClient::setTimeout()` takes a `uint16_t` (65 s max) and the endpoint is
  plain HTTP, no TLS. A non-2xx answer or a transport error is logged and the
  reading is dropped: there is no retry queue yet.
- The ingestion payload also carries the frame's `rssi`, taken from
  `info->rx_ctrl->rssi` in the receive callback and stored in `ReceivedPacket`.
  It is serialized as `null` when `rx_ctrl` is not available, so the server can
  tell "no signal data" from a real `0 dBm`.
- The receiver runs on a Waveshare ESP32-C3-Zero: the onboard WS2812 on GPIO10 is
  the status LED (blue = connecting, green = associated, red = failed). The
  NeoPixel library is the only external dependency of that project.
- The receiver loop is single-threaded and shared by the web server and the
  ingestion POST: serve HTTP first, process at most **one** queued packet per
  iteration, keep `INGESTION_TIMEOUT_MS` short (currently 3 s) and the loop delay
  at ~2 ms.
  A hung endpoint must delay `/wake` by one request, not by the whole backlog.
- Wake-on-LAN lives on `WOL_HTTP_PORT` from `secrets.h` (`GET /health`,
  `POST /wake` with a Bearer token from the same file) and sends the 102-byte
  magic packet five times, alternating subnet broadcast and unicast on
  `WOL_PORT`. It is the same contract the old standalone `esp32-wol-proxy`
  project exposed, so existing clients keep working.
  Every inbound request is logged as `HTTP <method> <uri> from <ip> -> <status>`;
  the outbound ingestion POSTs are logged separately by `IngestionClient`.
- Wake-on-LAN is behind `WOL_ENABLED` (default 1, `-DWOL_ENABLED=0` to disable):
  with 0 the `WolService` methods become no-ops, the web server is not compiled
  and flash drops by ~35 KB. `main.cpp` needs no `#if` — the class keeps the same
  interface in both builds.
- Early `return`s in `setup()` on init failure leave the board awake forever
  draining the battery; error paths that should retry later must still call
  `sleepForNextMeasurement()`.
- The ESP-NOW send callback uses the IDF 5.x / Arduino core 3.x signature
  `onDataSent(const wifi_tx_info_t*, esp_now_send_status_t)`. The older
  `(const uint8_t* mac_addr, ...)` signature from most tutorials will not compile.
- `send()` blocks up to 1 s waiting for the send callback; sends are expected
  to complete synchronously before sleep.
- The platform is a pinned pioarduino GitHub zip (PlatformIO platform 55.3.312,
  Arduino core 3.3.12, ESP-IDF 5.5.5), not the registry `espressif32` platform.
  Prefer pioarduino / Arduino-ESP32 3.x docs; registry-era snippets may be wrong.
- `ARDUINO_USB_MODE=1` and `ARDUINO_USB_CDC_ON_BOOT=1` keep `Serial` on native
  USB. Removing them moves `Serial` to UART0 and the monitor goes silent.
- Flash is ~77 % of the 1.31 MB app partition on the OLED env; check the
  `pio run` size report before adding large libraries.

## Layout

- `firmware/sensor-node/src/main.cpp` — boot sequence, `measureAndSend()`,
  channel recovery; display only under `#if HAS_DISPLAY`
- `firmware/sensor-node/src/config.h` — application parameters and per-room
  `DEVICE_ID`
- `firmware/sensor-node/src/board_config.h` — per-board pins and `HAS_DISPLAY`
- `firmware/sensor-node/src/sht31_sensor.*` — SHT31 I2C wrapper
- `firmware/sensor-node/src/espnow_sender.*` — ESP-NOW init, peer registration,
  blocking send
- `firmware/sensor-node/src/display.*` — generic SSD1306 72x40 wrapper over U8g2
  (SW I2C on GPIO5/6)
- `firmware/sensor-node/src/status_led.*` — per-board blink after a confirmed send
- `shared/sensor_data.h` — shared wire format
- `firmware/receiver/src/main.cpp` — Wi-Fi association, loop scheduling
- `firmware/receiver/src/config.h` — ingestion and status LED parameters, WOL
  packet layout; ports, token and target addresses are in `secrets.h`
- `firmware/receiver/src/espnow_receiver.*` — ESP-NOW receive callback, packet
  validation, FreeRTOS packet queue, serial log
- `firmware/receiver/src/ingestion.*` — JSON payload and HTTP POST
- `firmware/receiver/src/wol.*` — `/health` and `/wake` endpoints, magic packet
- `firmware/receiver/src/status_led.*` — C3-Zero WS2812 status indicator
- `firmware/receiver/src/secrets.example.h` — template; `secrets.h` is local and
  gitignored (Wi-Fi credentials and `AUTH_TOKEN`)
- `firmware/sensor-node/.pio/`, `.vscode/c_cpp_properties.json`,
  `compile_commands.json` are generated artifacts; never hand-edit them.
