#include <Arduino.h>

#include "board_config.h"
#include "config.h"
#include "espnow_sender.h"
#include "sensor_data.h"
#include "sht31_sensor.h"
#include "status_led.h"

#if HAS_DISPLAY
#include "display.h"
#endif

Sht31Sensor sensor;
EspNowSender sender;
StatusLed statusLed;

#if HAS_DISPLAY
Display display;
#endif

// Preserve the sequence number, uptime and channel across deep-sleep cycles.
RTC_DATA_ATTR uint32_t sequence = 1;
RTC_DATA_ATTR uint64_t uptimeOffsetMs = 0;
RTC_DATA_ATTR uint8_t lastChannel = DEFAULT_CHANNEL;
RTC_DATA_ATTR uint8_t scanBackoffCycles = 0;

SendResult tryChannel(
    const SensorData& data,
    uint8_t channel,
    uint32_t timeoutMs)
{
  if (!sender.setChannel(channel)) {
    return SendResult::DriverError;
  }

  return sender.send(data, timeoutMs);
}

bool sendWithChannelRecovery(const SensorData& data)
{
  for (uint8_t attempt = 0; attempt < SEND_ATTEMPTS; ++attempt) {
    SendResult result = tryChannel(data, lastChannel, SEND_TIMEOUT_MS);

    if (result == SendResult::Ok) {
      scanBackoffCycles = 0;
      return true;
    }

    if (result == SendResult::DriverError) {
      return false;
    }

    if (attempt + 1 < SEND_ATTEMPTS) {
      delay(SEND_RETRY_DELAY_MS);
    }
  }

  if (scanBackoffCycles > 0) {
    --scanBackoffCycles;
    Serial.printf(
        "ESP-NOW scan skipped, %u cycles left\n",
        scanBackoffCycles
    );
    return false;
  }

  for (uint8_t channel : CHANNEL_SCAN) {
    if (channel == lastChannel) {
      continue;
    }

    SendResult result = tryChannel(
        data,
        channel,
        CHANNEL_PROBE_TIMEOUT_MS
    );

    if (result == SendResult::Ok) {
      lastChannel = channel;
      scanBackoffCycles = 0;
      Serial.printf("ESP-NOW channel changed to %u\n", channel);
      return true;
    }

    if (result == SendResult::DriverError) {
      return false;
    }
  }

  scanBackoffCycles = SCAN_BACKOFF_CYCLES;
  Serial.println("ERROR: receiver not reached on any channel");
  return false;
}

bool measureAndSend()
{
  float temperature;
  float humidity;

  if (!sensor.read(temperature, humidity)) {
    Serial.println("WARNING: SHT31 read failed, retrying...");
    delay(100);

    if (!sensor.read(temperature, humidity)) {
      Serial.println("ERROR: Failed to read SHT31");
      return false;
    }
  }

  SensorData data{
    DEVICE_ID,
    sequence++,
    static_cast<uint32_t>((uptimeOffsetMs + millis()) / 1000),
    temperature,
    humidity
  };

  bool sent = sendWithChannelRecovery(data);

  if (sent) {
    Serial.printf(
        "Sent | device=%u | seq=%lu | uptime=%lus | channel=%u | "
        "temp=%.2f C | humidity=%.2f %%\n",
        static_cast<unsigned>(data.deviceId),
        static_cast<unsigned long>(data.sequence),
        static_cast<unsigned long>(data.uptime),
        static_cast<unsigned>(lastChannel),
        data.temperature,
        data.humidity
    );

    statusLed.blinkSent();
  } else {
    Serial.println("ERROR: ESP-NOW transmission failed");
  }

#if HAS_DISPLAY
  char line[24];

  snprintf(line, sizeof(line), "T %.1f C", data.temperature);
  display.text(0, 15, line);

  snprintf(line, sizeof(line), "H %.1f %%", data.humidity);
  display.text(0, 34, line);
#endif

  return sent;
}

void sleepForNextMeasurement()
{
#if DEBUG_KEEP_AWAKE
  Serial.println("DEBUG: deep sleep disabled, staying awake");
  Serial.flush();
  return;
#endif

  Serial.printf(
      "Entering deep sleep for %lu seconds...\n",
      static_cast<unsigned long>(SLEEP_INTERVAL_US / 1000000ULL)
  );
  Serial.flush();

#if HAS_DISPLAY
  if (!KEEP_DISPLAY_ON_DURING_SLEEP) {
    display.powerSave(true);
  }
#endif

  // millis() only covers the awake part of the cycle: deep sleep resets it, so
  // the programmed sleep time has to be added explicitly, otherwise uptime
  // counts radio-on time instead of wall-clock time.
  uptimeOffsetMs += millis() + SLEEP_INTERVAL_US / 1000ULL;

  esp_sleep_enable_timer_wakeup(SLEEP_INTERVAL_US);
  esp_deep_sleep_start();
}

void setup() {
  Serial.begin(115200);
  delay(BOOT_DELAY_MS);

  Serial.println();
  Serial.println("ESP32-C3 + SHT31");
  Serial.println("----------------");
  Serial.printf("Node: %u\n", static_cast<unsigned>(DEVICE_ID));

  statusLed.begin();

#if HAS_DISPLAY
  Serial.printf(
      "Display during sleep: %s\n",
      KEEP_DISPLAY_ON_DURING_SLEEP ? "on" : "off"
  );

  if (!display.begin(DISPLAY_CONTRAST_PERCENT)) {
    Serial.println("WARNING: display initialization failed");
  }
#endif

  if (!sensor.begin(SDA_PIN, SCL_PIN)) {
    Serial.println("ERROR: SHT31 non found!");
    Serial.println("Check wire connections SDA/SCL/VCC/GND.");
    sleepForNextMeasurement();
    return;
  }
  Serial.println("SHT31 initialized");

  if(!sender.begin(RECEIVER_MAC)){
    sleepForNextMeasurement();
    return;
  }
  Serial.println("ESP-NOW initialized");

  measureAndSend();
  sleepForNextMeasurement();
}

void loop() {
#if DEBUG_KEEP_AWAKE
  constexpr uint32_t INTERVAL_MS =
      static_cast<uint32_t>(SLEEP_INTERVAL_US / 1000ULL);

  Serial.printf(
      "DEBUG: next measurement in %lu s\n",
      static_cast<unsigned long>(INTERVAL_MS / 1000)
  );
  Serial.flush();

  delay(INTERVAL_MS);
  measureAndSend();
#endif
}
