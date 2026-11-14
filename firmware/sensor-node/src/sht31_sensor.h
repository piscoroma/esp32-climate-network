#pragma once

#include <Arduino.h>

class Sht31Sensor{
   
   public:
      bool begin(uint8_t sdaPin, uint8_t sclPin);
      bool read(float& temperature, float& humidity);
   
   private:
      bool initialized = false;

};
