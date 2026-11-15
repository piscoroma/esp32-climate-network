#pragma once

#include <stdint.h>

// Gateway Wi-Fi: the receiver must be associated to reach the ingestion
// endpoint. ESP-NOW then follows the router's channel, so the sensor nodes may
// need to rescan (they probe the channels after a failed send).
// SSID and password live in secrets.h, which is not committed.
constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 15000;
constexpr uint32_t WIFI_RETRY_INTERVAL_MS = 10000;

// One POST per received reading.
constexpr const char* INGESTION_URL = "http://192.168.1.175:9106/ingestion";
constexpr uint16_t INGESTION_TIMEOUT_MS = 5000;

// Readings buffered between the ESP-NOW callback and the ingestion loop, so a
// burst (or a slow POST) does not overwrite data that is still waiting.
constexpr uint8_t DATA_QUEUE_DEPTH = 8;

