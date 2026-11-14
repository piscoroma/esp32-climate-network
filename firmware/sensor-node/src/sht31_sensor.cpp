#include "sht31_sensor.h"

#include <Wire.h>
#include <Adafruit_SHT31.h>

namespace {

constexpr uint8_t SHT31_ADDRESS = 0x44;

Adafruit_SHT31 sht31;

}

bool Sht31Sensor::begin(uint8_t sdaPin, uint8_t sclPin){
   Wire.begin(sdaPin, sclPin);
   initialized = sht31.begin(SHT31_ADDRESS);
   return initialized;
}

bool Sht31Sensor::read(float& temperature, float& humidity){
   if(!initialized){
      return false;
   }
   
   temperature = sht31.readTemperature();
   humidity = sht31.readHumidity();

   if (isnan(temperature) || isnan(humidity)) {
      return false;
   }

   return true;
}