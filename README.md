# ESP32 Climate Network

Low-power indoor climate monitoring: battery-friendly ESP32-C3 sensor nodes push
temperature and humidity over ESP-NOW to a central ESP32-C3 gateway.

Every node wakes from deep sleep once a minute, reads an SHT31 over I2C, sends a
single fixed-size `SensorData` packet to the gateway, shows the reading on its
0.42" OLED (when the board has one) and goes back to sleep. No Wi-Fi
association, no MQTT broker, no cloud: ESP-NOW is connectionless and costs a few
milliseconds of radio time per measurement, which is what makes battery-powered
nodes practical.

## Status

| Component | Path | State |
|---|---|---|
| Sensor node | `firmware/sensor-node/` | Working — 3 boards, 6 build environments |
| Receiver / gateway | `firmware/receiver/` | ESP-NOW receive, HTTP ingestion, Wake-on-LAN proxy |
| Wire format | `shared/sensor_data.h` | Stable, pinned by `static_assert` |

## Architecture

```
  ESP32-C3 node  (SHT31 + OLED)  ──┐
  ESP32-C3 node  (SHT31)         ──┼──  ESP-NOW, SensorData 20 B  ──▶  ESP32-C3 gateway
  ESP32-C3 node  (SHT31)         ──┘                                   (room summary, planned)
```

The nodes do not talk to each other: each one is stateless apart from a few
`RTC_DATA_ATTR` variables (sequence number, cumulative uptime, last used
channel) that survive deep sleep.

## Hardware

| Env | Board | I2C SDA / SCL | Display | Status LED |
|---|---|---|---|---|
| `oled042` | ESP32-C3 devkit + 0.42" OLED module | GPIO2 / GPIO20 | SSD1306 72x40 | GPIO8, plain blue |
| `xiao-c3` | Seeed XIAO ESP32C3 | GPIO6 / GPIO7 | none | not configured |
| `c3-zero` | Waveshare ESP32-C3-Zero | GPIO4 / GPIO5 * | none | GPIO10, WS2812 |

\* Placeholder in `src/board_config.h`; set it to the actual wiring.

All boards are ESP32-C3 with 4 MB flash. A generic `esp32-c3-devkitm-1` board
definition is used for the OLED and C3-Zero builds — it only selects the target
and flash size, not the pinout.

**OLED board specifics.** The module carries an SSD1306 72x40 (EastRising) wired
to SDA = GPIO5 / SCL = GPIO6. The SHT31 is deliberately on GPIO2 / GPIO20 (left
header), and the ESP32-C3 has a single I2C controller: the panel is therefore
driven through U8g2 **software I2C**, so `Wire` stays dedicated to the sensor.
The red PWR LED is hardwired and cannot be switched off in software (~0.3 mA).
The blue user LED on GPIO8 blinks once per confirmed send: it can only blink
blue, a green blink requires the C3-Zero WS2812 or an external LED.

Power characteristics worth knowing per board:

| Board | Notes |
|---|---|
| XIAO ESP32C3 | ~44 µA; onboard charger, external antenna connector |
| OLED module | Red PWR LED always on; the panel keeps its GDDRAM in sleep |
| C3-Zero | WS2812 on GPIO10 draws ~1 mA even when dark |

## Repository layout

```
.
├── firmware/
│   ├── sensor-node/          ESP32-C3 node firmware (PlatformIO)
│   │   ├── src/
│   │   │   ├── main.cpp          boot sequence, measurement, channel recovery
│   │   │   ├── config.h          sleep interval, receiver MAC, DEVICE_ID
│   │   │   ├── board_config.h    per-board pins and HAS_DISPLAY
│   │   │   ├── sht31_sensor.*    SHT31 wrapper
│   │   │   ├── espnow_sender.*   ESP-NOW init, peer, blocking send
│   │   │   └── display.*         SSD1306 72x40 wrapper over U8g2
│   │   └── platformio.ini
│   └── receiver/             ESP32-C3 gateway firmware (PlatformIO)
│       ├── src/
│       │   ├── main.cpp          Wi-Fi association, loop scheduling
│       │   ├── config.h          ingestion, WOL and LED parameters
│       │   ├── secrets.example.h template for secrets.h (not committed)
│       │   ├── espnow_receiver.* ESP-NOW receive, queue, serial log
│       │   ├── ingestion.*       JSON payload and HTTP POST
│       │   ├── wol.*             /health and /wake endpoints, magic packet
│       │   └── status_led.*      C3-Zero WS2812 status indicator
│       └── platformio.ini
├── shared/
│   └── sensor_data.h         ESP-NOW wire format, included by both projects
├── esp32-climate-network.code-workspace
└── AGENTS.md                 Engineering notes and non-obvious constraints
```

## Build and flash

Requirements: [PlatformIO Core](https://docs.platformio.org/en/latest/core/installation/index.html).
The `pioarduino` platform (Arduino core 3.x / ESP-IDF 5.x) is downloaded
automatically on the first build. On this machine `pio` is not on `PATH`; it is
at `~/.platformio/penv/bin/pio`.

Builds are run from the repository root; `default_envs = oled042`, so a bare
`pio run` builds only the OLED target.

In VSCode, open `esp32-climate-network.code-workspace` from the repository root
to get both firmware projects and `shared/` in a single window, or open
`firmware/sensor-node` directly. PlatformIO IDE needs the folder that contains
`platformio.ini` to be part of the workspace: if you open only the repository
root, the extension has to be pointed at the nested project and the C/C++
extension will not find the generated `.vscode/c_cpp_properties.json`, so
IntelliSense shows missing includes.

The receiver does not build on a fresh clone until its Wi-Fi credentials exist:
copy `firmware/receiver/src/secrets.example.h` to `secrets.h` first (see
[Ingestion](#ingestion)).

```bash
# Build one environment
pio run -d firmware/sensor-node -e oled042        # OLED board
pio run -d firmware/sensor-node -e xiao-c3        # Seeed XIAO ESP32C3
pio run -d firmware/sensor-node -e c3-zero        # Waveshare ESP32-C3-Zero

# Keep the board awake and the USB port available (serial debugging)
pio run -d firmware/sensor-node -e oled042-debug

# Receiver / gateway
pio run -d firmware/receiver -e c3                 # ESP32-C3 gateway

# Flash and watch the logs
pio run -d firmware/sensor-node -e oled042 -t upload --upload-port /dev/ttyACM0
pio device monitor -d firmware/sensor-node -e oled042 -b 115200

pio run -d firmware/receiver -e c3 -t upload --upload-port /dev/ttyACM0
pio device monitor -d firmware/receiver -e c3 -b 115200
```

| Environment | Board | Deep sleep |
|---|---|---|
| `oled042` | OLED devkit | yes |
| `xiao-c3` | XIAO ESP32C3 | yes |
| `c3-zero` | C3-Zero | yes |
| `*-debug` | same boards | no, stays awake |
| `c3` (receiver) | ESP32-C3 devkit | never sleeps |

Flash usage is ~77 % of the 1.31 MB application partition on the OLED build —
check the size report before adding large libraries.

## Configuration

`firmware/sensor-node/src/config.h` holds the parameters shared by every node:

| Constant | Default | Meaning |
|---|---|---|
| `SLEEP_INTERVAL_US` | 60 s | Deep-sleep period between measurements |
| `RECEIVER_MAC` | `B0:A6:04:98:52:40` | ESP-NOW peer address |
| `DEVICE_ID` | `DeviceId::UFFICIO` | Room the node is installed in |
| `DISPLAY_CONTRAST_PERCENT` | `1` | OLED contrast, 0-100 |
| `KEEP_DISPLAY_ON_DURING_SLEEP` | `true` | `false` blanks the panel in sleep (A/B power tests) |
| `DEFAULT_CHANNEL` | 1 | First ESP-NOW channel tried |
| `CHANNEL_SCAN` | 1, 6, 11, 2, ... | Channel probe order after a failed send |

`DEVICE_ID` is edited per room and rebuilt before flashing; it is not a build
flag. `src/board_config.h` selects pins and display presence from the `-DBOARD_*`
macro of the environment — add a board by adding one `#elif` branch there plus
a matching `[env:*]` section in `platformio.ini`.

## What a cycle does

1. Wake from the deep-sleep timer, start the USB-CDC serial port.
2. Initialize the display (if any) and the SHT31; bail out to sleep on failure.
3. Initialize ESP-NOW, register the gateway as a peer.
4. Read the sensor (one retry after 100 ms on failure).
5. Send the packet on the cached channel up to `SEND_ATTEMPTS` times.
6. If no ACK, probe the other channels; a success caches the new channel, a full
   failure sets a backoff so the next wakes skip the scan.
7. Blink the status LED when the packet was acknowledged (`BLINK_ON_SEND`).
8. Draw the reading on the OLED.
9. Add the awake time plus the programmed sleep interval to the uptime offset,
   then start the timer wake-up.

The channel scan exists because ESP-NOW uses the current Wi-Fi channel: when the
gateway is an access-point client it may follow the router to another channel,
and the nodes have to find it again.

## Wire format

`shared/sensor_data.h` is the single source of truth for both the node and the
gateway. Packets are raw struct bytes — no serialization, same ABI on both ends,
fixed-width types only:

| Offset | Size | Field |
|---|---|---|
| 0 | 1 | `deviceId` (`DeviceId`) |
| 1 | 4 | `sequence` — increments every wake |
| 5 | 4 | `uptime` — cumulative seconds since first boot |
| 9 | 4 | `temperature` (°C, `float`) |
| 13 | 4 | `humidity` (%RH, `float`) |

`sizeof(SensorData)` is pinned to 20 bytes by a `static_assert`: changing the
struct breaks the build everywhere on purpose, so the receiver has to be updated
in the same commit.

`DeviceId` currently defines: `UFFICIO`, `CAMERA_DA_LETTO`, `SALOTTO`,
`CUCINA`, `BAGNO`.

## Ingestion

The gateway joins the local Wi-Fi network and POSTs every received reading:

```
POST http://192.168.1.175:9106/ingestion
Content-Type: application/json

{"device_id":"CAMERA DA LETTO","sequence":42,"uptime":2520,"temperature":21.34,"humidity":45.67,"rssi":-63}
```

| Field | Type | Meaning |
|---|---|---|
| `device_id` | string | room name derived from `DeviceId` |
| `sequence` | number | node sequence counter, incremented every wake |
| `uptime` | number | node cumulative seconds since first boot |
| `temperature` | number | °C |
| `humidity` | number | %RH |
| `rssi` | number or `null` | signal strength in dBm seen by the gateway; `null` when the radio metadata is missing |

Wi-Fi credentials are not in the repository. Copy the template and fill it in
before flashing:

```bash
cp firmware/receiver/src/secrets.example.h firmware/receiver/src/secrets.h
$EDITOR firmware/receiver/src/secrets.h
```

`secrets.h` is listed in `.gitignore`. The endpoint and the timeouts live in
`firmware/receiver/src/config.h`.

Two consequences worth knowing: associating the gateway to an access point moves
the ESP-NOW channel to the router's one, so the nodes may have to rescan before
they are heard again; and a failed POST is only logged — each reading is sent
once, there is no retry queue yet.

Packets are buffered in a FreeRTOS queue of `DATA_QUEUE_DEPTH` entries (8), so a
slow POST or several nodes reporting at the same time do not overwrite readings
that are still waiting. Anything beyond that depth is discarded and reported on
the serial console.

Retried sends (lost ACK, channel probe) deliver the same packet more than once.
The gateway keeps the last `SensorData` per sender and drops exact repeats,
reporting the count on the console. The comparison covers the whole struct, not
just `sequence`, so a node power cycle — which restarts `sequence` but also
`uptime` — is never mistaken for a duplicate. This is RAM-only and does not
replace an idempotent `(device_id, sequence)` key on the server side.

## Wake-on-LAN

The gateway doubles as a Wake-on-LAN proxy, so the homelab host can be woken from
another network:

```
GET  http://<gateway>:<port>/health   ->  {"status":"ok"}
POST http://<gateway>:<port>/wake     ->  {"status":"wake_packet_sent"}
```

`/wake` requires `Authorization: Bearer <token>`; the token is `AUTH_TOKEN` in
`secrets.h`. The magic packet (102 bytes) is sent five times, alternating between
the subnet broadcast address and a unicast address, on the UDP Wake-on-LAN port.
The listening port, the UDP port, the host to wake and those two addresses are
deployment data and live in `secrets.h` too (`WOL_HTTP_PORT`, `WOL_PORT`,
`WOL_TARGET_MAC`, `WOL_BROADCAST`, `WOL_UNICAST`), so they never end up in the
repository. The template uses 7766 for HTTP and the standard 9 for Wake-on-LAN.

```bash
curl -i http://<gateway>:<port>/health
curl -i -X POST http://<gateway>:<port>/wake -H 'Authorization: Bearer <token>'
```

Every request to the web service is logged on the serial console with the client
address and the status code (outbound ingestion POSTs are not logged this way):

```
HTTP GET /health from 192.168.1.20 -> 200
HTTP POST /wake from 192.168.1.20 -> 200
HTTP POST /wake from 192.168.1.20 -> 401
HTTP GET /nope from 192.168.1.20 -> 404
```

Both jobs share one core and one radio. The loop serves HTTP first and then
processes **at most one** queued reading per iteration, so an ingestion POST can
delay a `/wake` request by at most one request, never by the whole backlog.
ESP-NOW reception is unaffected either way: it runs in the Wi-Fi task and the
packets wait in the queue.

The proxy is optional: build with `-DWOL_ENABLED=0` (or set the macro to 0 in
`config.h`) to leave it out. That removes the web server and the UDP stack and
saves about 35 KB of flash.

## Roadmap

- [x] `firmware/receiver/` — ESP-NOW receive, packet validation and serial log
- [x] HTTP ingestion — one JSON POST per received reading
- [ ] Room-summary display on the gateway
- [ ] Retry queue for failed ingestion (a failed POST currently drops the reading)
- [ ] Real I2C pin pair for the Waveshare C3-Zero
- [ ] Documented power measurements per board (display on/off, sleep interval)

## Engineering notes

`AGENTS.md` collects the non-obvious constraints of this codebase: strapping
pins, why the display cannot share the hardware I2C bus, why `HAS_DISPLAY` must
stay a preprocessor macro, what breaks the serial monitor, and why debug builds
must never be left on a battery-powered node.

## License

MIT — see [LICENSE](LICENSE).
