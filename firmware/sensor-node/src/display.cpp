#include "display.h"

namespace {

uint8_t contrastToRaw(uint8_t percent) {
   if (percent > 100) {
      percent = 100;
   }

   return static_cast<uint8_t>(
       (static_cast<uint16_t>(percent) * 255 + 50) / 100);
}

}

Display::Display(uint8_t sclPin, uint8_t sdaPin)
    : u8g2(U8G2_R0, sclPin, sdaPin, U8X8_PIN_NONE) {}

bool Display::begin(uint8_t contrastPercent) {
   initialized = u8g2.begin();

   if (!initialized) {
      return false;
   }

   u8g2.setFont(u8g2_font_7x14B_tr);
   setContrast(contrastPercent);
   u8g2.clearBuffer();
   u8g2.sendBuffer();
   return true;
}

void Display::clear() {
   if (!initialized) {
      return;
   }

   u8g2.clearBuffer();
   u8g2.sendBuffer();
}

void Display::text(uint8_t x, uint8_t y, const char* str) {
   if (!initialized) {
      return;
   }

   u8g2.drawStr(x, y, str);
   u8g2.sendBuffer();
}

void Display::setContrast(uint8_t percent) {
   if (!initialized) {
      return;
   }

   u8g2.setContrast(contrastToRaw(percent));
}

void Display::powerSave(bool enable) {
   if (!initialized) {
      return;
   }

   u8g2.setPowerSave(enable);
}
