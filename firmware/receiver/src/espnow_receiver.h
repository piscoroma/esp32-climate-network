#pragma once

#include <Arduino.h>
#include <esp_now.h>

#include <sensor_data.h>

class EspNowReceiver {

   public:
      bool begin();
      bool getLatestData(SensorData& data);

      // Room name of a device, used in the serial log and in the ingestion
      // payload.
      static const char* deviceName(DeviceId deviceId);

   private:
      static void onDataReceived(
         const esp_now_recv_info_t* info,
         const uint8_t* data,
         int len
      );

      static void printSensorData(const SensorData& data);

};
