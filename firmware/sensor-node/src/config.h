#pragma once

#include <stdint.h>

#include "sensor_data.h"

// Set to 1 (or build a -debug env) to keep the board awake for serial debug.
#ifndef DEBUG_KEEP_AWAKE
#define DEBUG_KEEP_AWAKE 0
#endif

// Application parameters: same for every node (edit freely).

constexpr uint64_t SLEEP_INTERVAL_US = 60ULL * 1000000ULL;

// Longer boot delay in debug builds so the USB CDC monitor can attach.
constexpr uint32_t BOOT_DELAY_MS = DEBUG_KEEP_AWAKE ? 2000 : 500;

// ESP32 receiver MAC address (same for all nodes).
constexpr uint8_t RECEIVER_MAC[] = {
    //0xB0, 0xA6, 0x04, 0x98, 0x52, 0x40 // esp32-c6
    0x90, 0xE5, 0xB1, 0x6C, 0x3A, 0x94  // esp32-c3
};

// ESP-NOW channel recovery: retry the cached channel, then probe others.
constexpr uint32_t SEND_TIMEOUT_MS = 1000;
constexpr uint8_t SEND_ATTEMPTS = 3;
constexpr uint32_t SEND_RETRY_DELAY_MS = 100;
constexpr uint32_t CHANNEL_PROBE_TIMEOUT_MS = 300;
constexpr uint8_t SCAN_BACKOFF_CYCLES = 10;
constexpr uint8_t DEFAULT_CHANNEL = 1;
constexpr uint8_t CHANNEL_SCAN[] = {
    1, 6, 11, 2, 3, 4, 5, 7, 8, 9, 10, 12, 13
};

constexpr uint8_t DISPLAY_CONTRAST_PERCENT = 1;
constexpr bool KEEP_DISPLAY_ON_DURING_SLEEP = true;

// Change this when flashing a node for a different room.
constexpr DeviceId DEVICE_ID = DeviceId::UFFICIO;
