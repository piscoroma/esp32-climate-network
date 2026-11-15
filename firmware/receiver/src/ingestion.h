#pragma once

#include <sensor_data.h>

class IngestionClient {

   public:
      // POSTs one reading as JSON. Returns false when the request could not be
      // sent or the server answered with a non-2xx status.
      bool send(const SensorData& data);

};
