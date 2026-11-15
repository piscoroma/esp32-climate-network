#include "ingestion.h"

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFi.h>

#include "config.h"
#include "espnow_receiver.h"

bool IngestionClient::send(const SensorData& data)
{
   if (WiFi.status() != WL_CONNECTED) {
      Serial.println("ERROR: ingestion skipped, Wi-Fi not connected");
      return false;
   }

   // {"device_id":"UFFICIO","sequence":42,"uptime":2520,
   //  "temperature":21.34,"humidity":45.67,"rssi":-63}
   char rssi[8];
   if (packet.rssiValid) {
      snprintf(rssi, sizeof(rssi), "%d", packet.rssi);
   } else {
      snprintf(rssi, sizeof(rssi), "null");
   }

   char payload[192];
   const int length = snprintf(
      payload,
      sizeof(payload),
      "{\"device_id\":\"%s\",\"sequence\":%lu,\"uptime\":%lu,"
      "\"temperature\":%.2f,\"humidity\":%.2f,\"rssi\":%s}",
      EspNowReceiver::deviceName(packet.data.deviceId),
      static_cast<unsigned long>(packet.data.sequence),
      static_cast<unsigned long>(packet.data.uptime),
      packet.data.temperature,
      packet.data.humidity,
      rssi
   );

   if (length <= 0 || length >= static_cast<int>(sizeof(payload))) {
      Serial.println("ERROR: ingestion payload does not fit the buffer");
      return false;
   }

   WiFiClient client;
   HTTPClient http;

   http.setTimeout(INGESTION_TIMEOUT_MS);
   http.setConnectTimeout(INGESTION_TIMEOUT_MS);

   if (!http.begin(client, INGESTION_URL)) {
      Serial.println("ERROR: invalid ingestion URL");
      return false;
   }

   http.addHeader("Content-Type", "application/json");

   const int status = http.POST(
      reinterpret_cast<uint8_t*>(payload),
      static_cast<size_t>(length)
   );

   if (status <= 0) {
      Serial.printf(
         "ERROR: ingestion POST failed: %d %s\n",
         status,
         HTTPClient::errorToString(status).c_str()
      );
      http.end();
      return false;
   }

   http.end();

   if (status < 200 || status >= 300) {
      Serial.printf("WARNING: ingestion returned HTTP %d\n", status);
      return false;
   }

   Serial.printf("Ingestion ok | HTTP %d | %d bytes\n", status, length);
   return true;
}
