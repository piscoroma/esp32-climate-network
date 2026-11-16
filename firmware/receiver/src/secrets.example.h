#pragma once

#include <IPAddress.h>
#include <stdint.h>

// Copy this file to secrets.h and fill in your network.
// secrets.h is listed in .gitignore: never commit real credentials.

constexpr const char* WIFI_SSID = "your-ssid";
constexpr const char* WIFI_PASSWORD = "your-password";

// Bearer token required by POST /wake.
constexpr const char* AUTH_TOKEN = "your-token";

// Port of the web service and UDP port of the magic packet.
constexpr uint16_t WOL_HTTP_PORT = 7766;
constexpr uint16_t WOL_PORT = 9;

// Host to wake, its subnet broadcast and its unicast address.
constexpr uint8_t WOL_TARGET_MAC[6] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const IPAddress WOL_BROADCAST(192, 168, 1, 255);
const IPAddress WOL_UNICAST(192, 168, 1, 100);
