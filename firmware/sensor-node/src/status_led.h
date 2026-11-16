#pragma once

#include <Arduino.h>

#include "board_config.h"

// Visual feedback on the board LED. Does nothing when the board has no status
// LED (HAS_STATUS_LED 0).
class StatusLed {

   public:
      void begin();

      // Blinks once to confirm a reading was delivered. Blocks for durationMs.
      void blinkSent(uint16_t durationMs = 60);

};
