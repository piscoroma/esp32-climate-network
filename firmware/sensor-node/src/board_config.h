#pragma once

#include <stdint.h>

// Selected by the PlatformIO env with -DBOARD_*.
// Add a new board here (pins and display presence only).

#if defined(BOARD_OLED_042)
constexpr uint8_t SDA_PIN = 2;
constexpr uint8_t SCL_PIN = 20;
#define HAS_DISPLAY 1
#elif defined(BOARD_XIAO_C3)
constexpr uint8_t SDA_PIN = 6;   // D4
constexpr uint8_t SCL_PIN = 7;   // D5
#define HAS_DISPLAY 0
#elif defined(BOARD_C3_ZERO)
constexpr uint8_t SDA_PIN = 4;   // adjust to your wiring
constexpr uint8_t SCL_PIN = 5;
#define HAS_DISPLAY 0
#else
#error "Define one BOARD_* macro (BOARD_OLED_042, BOARD_XIAO_C3, BOARD_C3_ZERO)"
#endif
