#include "status_led.h"

#if HAS_STATUS_LED

namespace {

#if STATUS_LED_RGB

void setRgb(uint8_t red, uint8_t green, uint8_t blue)
{
   rgbLedWrite(STATUS_LED_PIN, red, green, blue);
}

#else

void setLed(bool on)
{
   const uint8_t level = STATUS_LED_ACTIVE_LOW ? !on : on;
   digitalWrite(STATUS_LED_PIN, level);
}

#endif

} // namespace

void StatusLed::begin()
{
#if !STATUS_LED_RGB
   pinMode(STATUS_LED_PIN, OUTPUT);
   setLed(false);
#endif
}

void StatusLed::blinkSent(uint16_t durationMs)
{
#if STATUS_LED_RGB
   setRgb(0, 255, 0);
   delay(durationMs);
   setRgb(0, 0, 0);
#else
   setLed(true);
   delay(durationMs);
   setLed(false);
#endif
}

#else

void StatusLed::begin() {}

void StatusLed::blinkSent(uint16_t) {}

#endif
