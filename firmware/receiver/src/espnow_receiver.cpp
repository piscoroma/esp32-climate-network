#include "espnow_receiver.h"

#include <WiFi.h>

#include "config.h"

namespace {

QueueHandle_t packetQueue = nullptr;
std::atomic<uint32_t> droppedPackets{0};

void printMacAddress(const uint8_t* mac)
{
   Serial.printf(
      "%02X:%02X:%02X:%02X:%02X:%02X",
      mac[0], mac[1], mac[2],
      mac[3], mac[4], mac[5]
   );
}

} // namespace

const char* EspNowReceiver::deviceName(DeviceId deviceId)
{
   switch (deviceId) {
      case DeviceId::UFFICIO:
         return "UFFICIO";
      case DeviceId::CAMERA_DA_LETTO:
         return "CAMERA_DA_LETTO";
      case DeviceId::SALOTTO:
         return "SALOTTO";
      case DeviceId::CUCINA:
         return "CUCINA";
      case DeviceId::BAGNO:
         return "BAGNO";
      default:
         return "UNKNOWN";
   }
}

bool EspNowReceiver::begin()
{
   WiFi.mode(WIFI_STA);

   // Modem sleep drops ESP-NOW frames between wake-ups.
   WiFi.setSleep(false);

   uint8_t mac[6];
   WiFi.macAddress(mac);

   Serial.print("Receiver MAC: ");
   printMacAddress(mac);
   Serial.println();

   // Same value, ready to paste into the sensor nodes' config.h.
   Serial.print("Sensor node RECEIVER_MAC = { ");
   for (size_t i = 0; i < sizeof(mac); ++i) {
      Serial.printf("0x%02X%s", mac[i], i + 1 < sizeof(mac) ? ", " : "");
   }
   Serial.println(" }");

   Serial.printf("Channel: %u\n", WiFi.channel());

   packetQueue = xQueueCreate(DATA_QUEUE_DEPTH, sizeof(ReceivedPacket));
   if (packetQueue == nullptr) {
      Serial.println("ERROR: Failed to create the receive queue");
      return false;
   }

   if (esp_now_init() != ESP_OK) {
      Serial.println("ERROR: ESP-NOW initialization failed");
      return false;
   }

   if (esp_now_register_recv_cb(onDataReceived) != ESP_OK) {
      Serial.println("ERROR: Failed to register receive callback");
      return false;
   }

   return true;
}

// Runs in the Wi-Fi task: no serial output and no network calls here, only a
// copy into the queue. With a single slot any packet arriving while the loop is
// busy with a POST would be overwritten and lost.
void EspNowReceiver::onDataReceived(
   const esp_now_recv_info_t* info,
   const uint8_t* data,
   int len)
{
   if (len != static_cast<int>(sizeof(SensorData))) {
      Serial.printf(
         "ERROR: Invalid packet size. Expected %u, received %d\n",
         static_cast<unsigned>(sizeof(SensorData)),
         len
      );
      return;
   }

   ReceivedPacket packet{};
   memcpy(&packet.data, data, sizeof(packet.data));
   memcpy(packet.sourceMac, info->src_addr, sizeof(packet.sourceMac));

   packet.rssiValid = info->rx_ctrl != nullptr;
   packet.rssi = packet.rssiValid ? info->rx_ctrl->rssi : 0;

   if (xQueueSend(packetQueue, &packet, 0) != pdTRUE) {
      droppedPackets.fetch_add(1, std::memory_order_relaxed);
   }
}

bool EspNowReceiver::dequeue(ReceivedPacket& packet)
{
   if (packetQueue == nullptr) {
      return false;
   }

   return xQueueReceive(packetQueue, &packet, 0) == pdTRUE;
}

uint32_t EspNowReceiver::takeDroppedPackets()
{
   return droppedPackets.exchange(0, std::memory_order_relaxed);
}

void EspNowReceiver::logPacket(const ReceivedPacket& packet)
{
   Serial.print("Packet received from ");
   printMacAddress(packet.sourceMac);

   if (packet.rssiValid) {
      Serial.printf(" | rssi=%d dBm", packet.rssi);
   } else {
      Serial.print(" | rssi=n/a");
   }

   Serial.printf(" | channel=%u\n", WiFi.channel());

   printSensorData(packet.data);
}

void EspNowReceiver::printSensorData(const SensorData& data)
{
   Serial.printf(
      "Received | device=%s (%u) | seq=%lu | uptime=%lus | "
      "temp=%.2f C | humidity=%.2f %%\n",
      deviceName(data.deviceId),
      static_cast<unsigned>(data.deviceId),
      static_cast<unsigned long>(data.sequence),
      static_cast<unsigned long>(data.uptime),
      data.temperature,
      data.humidity
   );
}
