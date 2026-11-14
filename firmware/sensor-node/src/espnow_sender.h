#pragma once

#include <Arduino.h>
#include <esp_now.h>
#include <sensor_data.h>

enum class SendResult : uint8_t {
   Ok,
   NoAck,
   Timeout,
   DriverError
};

class EspNowSender{

   public:
      bool begin(const uint8_t* receiverMacAddress);
      bool setChannel(uint8_t channel);
      SendResult send(const SensorData& data, uint32_t timeoutMs = 1000);

   private:
      uint8_t receiverMac[6]{};
      uint8_t currentChannel = 1;

};
