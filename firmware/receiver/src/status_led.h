#pragma once

#include <Adafruit_NeoPixel.h>
#include <Arduino.h>

#include "config.h"

// Onboard WS2812 of the C3-Zero, used as a connectivity indicator.
class StatusLed {

   public:
      void begin();
      void connecting();
      void connected();
      void disconnected();

   private:
      void set(uint8_t red, uint8_t green, uint8_t blue);

      Adafruit_NeoPixel led{1, STATUS_LED_PIN, NEO_GRB + NEO_KHZ800};

};
