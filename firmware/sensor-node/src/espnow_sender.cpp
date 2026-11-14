#include <espnow_sender.h>

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

namespace {

volatile bool sendCompleted = false;
volatile esp_now_send_status_t sendStatus = ESP_NOW_SEND_FAIL;

void printMacAddress(const uint8_t* mac)
{
   Serial.printf(
      "%02X:%02X:%02X:%02X:%02X:%02X",
      mac[0], mac[1], mac[2],
      mac[3], mac[4], mac[5]
   );
}

// Called when ESP-NOW finishes attempting to send a packet.
void onDataSent(
    const wifi_tx_info_t* txInfo,
    esp_now_send_status_t status)
{
    (void)txInfo;
    sendStatus = status;
    sendCompleted = true;
}

} // namespace

bool EspNowSender::begin(const uint8_t* receiverMacAddress){
   memcpy(receiverMac, receiverMacAddress, 6);

   WiFi.mode(WIFI_STA);

   if (esp_now_init() != ESP_OK) {
      Serial.println("ERROR: ESP-NOW initialization failed");
      return false;
   }

   if (esp_now_register_send_cb(onDataSent) != ESP_OK) {
      Serial.println("ERROR: Failed to register send callback");
      return false;
   }

   esp_now_peer_info_t peerInfo{};

   memcpy(peerInfo.peer_addr, receiverMac, 6);
   peerInfo.channel = 0;
   peerInfo.encrypt = false;

   esp_err_t result = esp_now_add_peer(&peerInfo);
   
   if (result == ESP_OK) {
      Serial.print("ESP-NOW peer added: ");
      printMacAddress(receiverMac);
      Serial.print("\n");

   } else if (result == ESP_ERR_ESPNOW_EXIST) {
      Serial.print("ESP-NOW peer already exists: ");
      printMacAddress(receiverMac);
      Serial.print("\n");
      
   } else {
      Serial.printf(
         "ERROR: Failed to add ESP-NOW peer: %s (0x%04X)\n",
         esp_err_to_name(result),
         static_cast<unsigned>(result)
      );
      return false;
   }

   return true;

}

bool EspNowSender::setChannel(uint8_t channel){
   if (channel < 1 || channel > 13) {
      Serial.printf("ERROR: Invalid ESP-NOW channel: %u\n", channel);
      return false;
   }

   esp_err_t result = esp_wifi_set_channel(
      channel,
      WIFI_SECOND_CHAN_NONE
   );

   if (result != ESP_OK) {
      Serial.printf(
         "ERROR: Failed to set ESP-NOW channel %u: %s (0x%04X)\n",
         channel,
         esp_err_to_name(result),
         static_cast<unsigned>(result)
      );
      return false;
   }

   currentChannel = channel;
   return true;
}

SendResult EspNowSender::send(const SensorData& data, uint32_t timeoutMs){
   
   sendCompleted = false;
   sendStatus = ESP_NOW_SEND_FAIL;

   esp_err_t result = esp_now_send(
      receiverMac,
      reinterpret_cast<const uint8_t*>(&data),
      sizeof(data)
   );

   if (result != ESP_OK) {
      Serial.printf(
         "ERROR: ESP-NOW send failed: %s (0x%04X)\n",
         esp_err_to_name(result),
         static_cast<unsigned>(result)
      );
      return SendResult::DriverError;
   }

   const uint32_t start = millis();

   while (
      !sendCompleted &&
      static_cast<uint32_t>(millis() - start) < timeoutMs
   ) {
      delay(1);
   }

   if (!sendCompleted) {
      Serial.printf(
         "ERROR: ESP-NOW send timed out on channel %u\n",
         currentChannel
      );
      return SendResult::Timeout;
   }

   if (sendStatus != ESP_NOW_SEND_SUCCESS) {
      Serial.printf(
         "WARNING: ESP-NOW delivery not confirmed on channel %u\n",
         currentChannel
      );
      return SendResult::NoAck;
   }

   return SendResult::Ok;
}
