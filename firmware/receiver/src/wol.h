#pragma once

#include <Arduino.h>

#include "config.h"

#if WOL_ENABLED
#include "secrets.h"

#include <WebServer.h>
#include <WiFiUdp.h>
#endif

// Wake-on-LAN proxy: HTTP endpoints used to wake the homelab host from another
// network or from a phone. With WOL_ENABLED 0 the methods are no-ops and the
// web server is not compiled in.
class WolService {

   public:
      void begin();

      // Must be called from loop() to serve requests.
      void handleClient();

#if WOL_ENABLED
   private:
      void handleHealth();
      void handleWake();
      void handleNotFound();
      bool isAuthorized();
      void sendWakeOnLan();
      void sendMagicPacket(const uint8_t* packet, const IPAddress& destination);
      void logRequest(int status);

      WebServer server{WOL_HTTP_PORT};
      WiFiUDP udp;
#endif

};
