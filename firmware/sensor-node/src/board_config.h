#pragma once

#include <stdint.h>

// Selected by the PlatformIO env with -DBOARD_*.
// Add a new board here (pins, display and status LED only).

#if defined(BOARD_OLED_042)
constexpr uint8_t SDA_PIN = 2;
constexpr uint8_t SCL_PIN = 20;
#define HAS_DISPLAY 1
// Onboard user LED on GPIO8, active low. It can blink, but it is blue: a green
// blink needs the WS2812 of the C3-Zero or an external LED.
#define HAS_STATUS_LED 1
#define STATUS_LED_RGB 0
constexpr uint8_t STATUS_LED_PIN = 8;
constexpr bool STATUS_LED_ACTIVE_LOW = true;
#elif defined(BOARD_XIAO_C3)
constexpr uint8_t SDA_PIN = 6;   // D4
constexpr uint8_t SCL_PIN = 7;   // D5
#define HAS_DISPLAY 0
// No confirmed user LED wiring in the variant: set the pin and flip
// HAS_STATUS_LED to 1 if your board has one.
#define HAS_STATUS_LED 0
#define STATUS_LED_RGB 0
constexpr uint8_t STATUS_LED_PIN = 8;
constexpr bool STATUS_LED_ACTIVE_LOW = true;
#elif defined(BOARD_C3_ZERO)
constexpr uint8_t SDA_PIN = 2;
constexpr uint8_t SCL_PIN = 0;
#define HAS_DISPLAY 0
// Onboard WS2812 on GPIO10: the blink is green.
#define HAS_STATUS_LED 1
#define STATUS_LED_RGB 1
constexpr uint8_t STATUS_LED_PIN = 10;
constexpr bool STATUS_LED_ACTIVE_LOW = false;
#else
#error "Define one BOARD_* macro (BOARD_OLED_042, BOARD_XIAO_C3, BOARD_C3_ZERO)"
#endif
