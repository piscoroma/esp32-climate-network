#pragma once

#include <stdint.h>

enum class DeviceId : uint8_t {
   UFFICIO = 1,
   CAMERA_DA_LETTO = 2,
   SALOTTO = 3,
   CUCINA = 4,
   BAGNO = 5
};

struct SensorData {
   DeviceId deviceId;
   uint32_t sequence;
   uint32_t uptime;
   float temperature;
   float humidity;
};

static_assert(sizeof(SensorData) == 20,
              "SensorData wire format changed: update the receiver too");

