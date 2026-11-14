#pragma once

#include <Arduino.h>
#include <U8g2lib.h>

class Display {
 public:
  // Defaults are the onboard 0.42" OLED wiring (SCL = GPIO6, SDA = GPIO5).
  explicit Display(uint8_t sclPin = 6, uint8_t sdaPin = 5);

  // contrastPercent is 0-100 and is applied right after initialization.
  bool begin(uint8_t contrastPercent = 80);
  void clear();
  void text(uint8_t x, uint8_t y, const char* str);
  // 0-100; values above 100 are clamped.
  void setContrast(uint8_t percent);
  void powerSave(bool enable);

 private:
  U8G2_SSD1306_72X40_ER_F_SW_I2C u8g2;
  bool initialized = false;
};
