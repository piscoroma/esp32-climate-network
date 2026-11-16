#include <Arduino.h>
#include <WiFi.h>

#include "config.h"
#include "espnow_receiver.h"
#include "ingestion.h"
#include "secrets.h"
#include "status_led.h"

EspNowReceiver receiver;
IngestionClient ingestion;
StatusLed statusLed;

uint32_t lastWifiAttemptMs = 0;

void connectWifi()
{
  statusLed.connecting();

  Serial.printf("Wi-Fi: connecting to \"%s\"...\n", WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  const uint32_t start = millis();

  while (WiFi.status() != WL_CONNECTED &&
         static_cast<uint32_t>(millis() - start) < WIFI_CONNECT_TIMEOUT_MS) {
    delay(250);
    Serial.print('.');
  }

  Serial.println();
  lastWifiAttemptMs = millis();

  if (WiFi.status() == WL_CONNECTED) {
    statusLed.connected();

    Serial.printf(
        "Wi-Fi: connected | ip=%s | channel=%u\n",
        WiFi.localIP().toString().c_str(),
        static_cast<unsigned>(WiFi.channel()));
  } else {
    statusLed.disconnected();

    Serial.println("ERROR: Wi-Fi connection failed, retrying in background");
  }
}

// A failed association at boot is not fatal: ESP-NOW keeps working on the
// current channel while the Wi-Fi stack retries.
void keepWifiAlive()
{
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  if (static_cast<uint32_t>(millis() - lastWifiAttemptMs) <
      WIFI_RETRY_INTERVAL_MS) {
    return;
  }

  lastWifiAttemptMs = millis();
  statusLed.connecting();
  Serial.println("Wi-Fi: reconnecting...");
  WiFi.reconnect();
}

void setup() {
  Serial.begin(115200);
  delay(500);

  statusLed.begin();
  Serial.println();
  Serial.println("ESP32-C3 ESP-NOW receiver");
  Serial.println("-------------------------");

  connectWifi();

  if (!receiver.begin()) {
    Serial.println("ERROR: receiver initialization failed");
    return;
  }

  Serial.println("Waiting for SensorData packets...");
}

void loop() {
  // Serve HTTP first: it must not wait for the ingestion POSTs.
  wol.handleClient();

  // One reading per iteration, so a backlog cannot starve the web server.
  ReceivedPacket packet;
  if (receiver.dequeue(packet)) {
    receiver.logPacket(packet);
    ingestion.send(packet);
  }

  const uint32_t dropped = receiver.takeDroppedPackets();
  if (dropped > 0) {
    Serial.printf(
        "WARNING: %lu packet(s) discarded, receive queue full\n",
        static_cast<unsigned long>(dropped));
  }

  keepWifiAlive();
  delay(2);
}
