#pragma once

#include <Arduino.h>
#include <esp_now.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include <atomic>

#include <sensor_data.h>

struct ReceivedPacket {
   SensorData data;
   uint8_t sourceMac[6];
   int8_t rssi;
   bool rssiValid;
};

class EspNowReceiver {

   public:
      bool begin();

      // Pops the oldest queued packet. Returns false when the queue is empty.
      bool dequeue(ReceivedPacket& packet);

      // Prints one packet to the serial console.
      void logPacket(const ReceivedPacket& packet);

      // Packets discarded because the queue was full, reset to zero on read.
      uint32_t takeDroppedPackets();

      // Exact duplicates already queued once, reset to zero on read.
      uint32_t takeDuplicatePackets();

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
