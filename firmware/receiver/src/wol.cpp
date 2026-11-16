#include "wol.h"

#if WOL_ENABLED

#include <cstring>

#include "secrets.h"

namespace {

const char* HEADER_KEY_AUTHORIZATION[] = {
   "Authorization"
};

const char* methodName(HTTPMethod method)
{
   switch (method) {
      case HTTP_GET:
         return "GET";
      case HTTP_POST:
         return "POST";
      case HTTP_PUT:
         return "PUT";
      case HTTP_DELETE:
         return "DELETE";
      case HTTP_HEAD:
         return "HEAD";
      case HTTP_OPTIONS:
         return "OPTIONS";
      default:
         return "OTHER";
   }
}

} // namespace

void WolService::begin()
{
   server.on("/health", HTTP_GET, [this]() {
      handleHealth();
   });

   server.on("/wake", HTTP_POST, [this]() {
      handleWake();
   });

   server.onNotFound([this]() {
      handleNotFound();
   });

   server.collectHeaders(HEADER_KEY_AUTHORIZATION, 1);
   server.begin();

   Serial.printf("WOL: HTTP server on port %u\n", WOL_HTTP_PORT);
}

void WolService::handleClient()
{
   server.handleClient();
}

void WolService::handleHealth()
{
   server.send(200, "application/json", "{\"status\":\"ok\"}");
   logRequest(200);
}

void WolService::handleWake()
{
   if (!isAuthorized()) {
      server.send(401, "application/json", "{\"error\":\"unauthorized\"}");
      logRequest(401);
      return;
   }

   sendWakeOnLan();
   server.send(200, "application/json", "{\"status\":\"wake_packet_sent\"}");
   logRequest(200);
}

void WolService::handleNotFound()
{
   server.send(404, "application/json", "{\"error\":\"not_found\"}");
   logRequest(404);
}

bool WolService::isAuthorized()
{
   if (!server.hasHeader("Authorization")) {
      return false;
   }

   return server.header("Authorization") == String("Bearer ") + AUTH_TOKEN;
}

void WolService::logRequest(int status)
{
   Serial.printf(
      "HTTP %s %s from %s -> %d\n",
      methodName(server.method()),
      server.uri().c_str(),
      server.client().remoteIP().toString().c_str(),
      status
   );
}

void WolService::sendWakeOnLan()
{
   uint8_t packet[WOL_MAGIC_PACKET_SIZE];

   memset(packet, 0xFF, 6);

   for (size_t i = 0; i < 16; ++i) {
      memcpy(packet + 6 + (i * 6), WOL_TARGET_MAC, 6);
   }

   // Alternate broadcast and unicast, starting and ending with broadcast, like
   // the original proxy did.
   for (uint8_t round = 0; round < WOL_SEND_ROUNDS; ++round) {
      const IPAddress& destination =
         (round % 2 == 0) ? WOL_BROADCAST : WOL_UNICAST;

      sendMagicPacket(packet, destination);

      if (round + 1 < WOL_SEND_ROUNDS) {
         delay(WOL_SEND_INTERVAL_MS);
      }
   }

   Serial.println("WOL: magic packet sent");
}

void WolService::sendMagicPacket(
   const uint8_t* packet,
   const IPAddress& destination)
{
   udp.beginPacket(destination, WOL_PORT);
   udp.write(packet, WOL_MAGIC_PACKET_SIZE);
   udp.endPacket();
}

#else // WOL_ENABLED

void WolService::begin() {}

void WolService::handleClient() {}

#endif // WOL_ENABLED
