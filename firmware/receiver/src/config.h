#pragma once

#include <stdint.h>

// Gateway Wi-Fi: the receiver must be associated to reach the ingestion
// endpoint. ESP-NOW then follows the router's channel, so the sensor nodes may
// need to rescan (they probe the channels after a failed send).
// SSID and password live in secrets.h, which is not committed.
constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 15000;
constexpr uint32_t WIFI_RETRY_INTERVAL_MS = 10000;

// One POST per received reading. Kept short: the loop is single-threaded, so a
// hung endpoint delays the Wake-on-LAN endpoints too.
constexpr const char* INGESTION_URL = "http://192.168.1.175:9106/ingestion";
constexpr uint16_t INGESTION_TIMEOUT_MS = 3000;

// Readings buffered between the ESP-NOW callback and the ingestion loop, so a
// burst (or a slow POST) does not overwrite data that is still waiting.
constexpr uint8_t DATA_QUEUE_DEPTH = 8;

// ---------------------------------------------------------------- Wake-on-LAN
// Set WOL_ENABLED to 0 (or build with -DWOL_ENABLED=0) to leave the proxy out
// of the firmware entirely.
#ifndef WOL_ENABLED
#define WOL_ENABLED 1
#endif

// HTTP endpoints: GET /health and POST /wake (Bearer token in secrets.h).
// The listening port, the UDP port, the host to wake and its addresses are
// deployment data: they live in secrets.h, which is not committed.
constexpr size_t WOL_MAGIC_PACKET_SIZE = 102;
constexpr uint8_t WOL_SEND_ROUNDS = 5;
constexpr uint16_t WOL_SEND_INTERVAL_MS = 100;

// The host to wake and its addresses are deployment data: they live in
// secrets.h (WOL_TARGET_MAC, WOL_BROADCAST, WOL_UNICAST), which is not
// committed.

// ------------------------------------------------------------------ Status LED
// Onboard WS2812 of the Waveshare ESP32-C3-Zero.
constexpr uint8_t STATUS_LED_PIN = 10;
constexpr uint8_t STATUS_LED_BRIGHTNESS = 1;
