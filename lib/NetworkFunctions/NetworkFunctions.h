#if defined (ESP8266)
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#elif defined(ESP32)
#include <WiFi.h>
#include "ESPmDNS.h"
#else
#error The board must be ESP8266 or ESP32
#endif // ESP
#include <WiFiUdp.h>

boolean startNetwork(const char* ssid,const char* password,const char* device_name, IPAddress ip, IPAddress gateway, IPAddress subnet, IPAddress dns1 = (uint32_t)0x00000000, IPAddress dns2 = (uint32_t)0x00000000);