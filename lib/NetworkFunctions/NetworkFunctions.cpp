#include <NetworkFunctions.h>

boolean startNetwork(const char* ssid,const char* password,const char* device_name, IPAddress ip, IPAddress gateway, IPAddress subnet, IPAddress dns1, IPAddress dns2) {
    WiFi.persistent( false );
    WiFi.mode(WIFI_STA);
    Serial.println(ip);
    WiFi.config( ip, gateway, subnet, dns1, dns2 );
    WiFi.begin(ssid, password);
    for (uint8_t i = 0; i<50; i++) {
        if(WiFi.waitForConnectResult() == WL_CONNECTED) {
            break;
        }
        delay(10);
    }

    if(WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.println("Wifi connection failed");
        return false;
    }

    if (!MDNS.begin(device_name)) {
        Serial.println("MDNS failed");
    }

    return true;
}