#include "status_led.h"

void StatusLed::begin()
{
   led.begin();
   led.setBrightness(STATUS_LED_BRIGHTNESS);
   led.clear();
   led.show();
}

void StatusLed::connecting()
{
   set(0, 0, 255);
}

void StatusLed::connected()
{
   set(0, 255, 0);
}

void StatusLed::disconnected()
{
   set(255, 0, 0);
}

void StatusLed::set(uint8_t r, uint8_t g, uint8_t b)
{
   led.setPixelColor(0, g, r, b);
   //led.setPixelColor(0, led.Color(red, green, blue));
   led.show();
}
