#include <BaseConfigDef.h>
#include "BaseConfig.h"

#include "WiFi.h" 
#include "driver/adc.h"
#include <esp_wifi.h>
#include <esp_bt.h>

#include <Arduino.h>
#include <Wire.h>

#include <U8g2lib.h>

#include <RemoteDebug.h>
#include <INTERVAL.h>
#include <MqttController.hpp>

#include <BMEFunctions.h>
#include <NetworkFunctions.h>
#include <OTAFunctions.h>
#include <MqttFunctions.h>
#include <RemoteDebugFunctions.h>

#define uS_TO_S_FACTOR 1000000  
#define TIME_TO_SLEEP  1    

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE); //, /* clock=*/ 16, /* data=*/ 17);   // ESP32 Thing, HW I2C with pin remapping

RemoteDebug Debug;

WiFiClient espClient;

PubSubClient pubsubclient(mqtt_server, 1883, espClient);
MqttController mqtt_controller(pubsubclient,device_name,mqtt_user,mqtt_pw);
void mqtt_callback_func(const char* topic, const byte* payload, unsigned int length) { mqtt_controller.callback(topic, payload,length); }

Bsec bme;

uint32_t loopstart;
uint32_t loopduration = 0;
char m_str[11] = {};

IPAddress ip( 10,166,70,LOWIP );
IPAddress gateway( 10, 166, 64, 1 );
IPAddress subnet( 255, 255, 240, 0 );
IPAddress dns1( 10, 166, 64, 1 );

void writeLeftBottom(const char* m_str) {
  u8g2.setColorIndex(0);  
  u8g2.drawBox(0,58,64,6);
  u8g2.setFont(u8g2_font_4x6_mn);
  u8g2.setColorIndex(1);  
  u8g2.drawStr(0,64,m_str); 
  u8g2.setColorIndex(1);
  u8g2.setFont(u8g2_font_7x14_mf);
}

void setup(void) {
  adc_power_off();

  Serial.begin(115200);	
  Wire.begin(); 

  u8g2.begin();
  u8g2.setDisplayRotation(U8G2_R2);
  u8g2.clearBuffer();          

  writeLeftBottom("!");
  u8g2.sendBuffer();
  Serial.println("Starting network");
  startNetwork(ssid,password,device_name,ip,gateway,subnet,dns1);
  writeLeftBottom("!!");
  u8g2.sendBuffer();
  Serial.println(millis());
  // Start network services
  startOTA(device_name, ota_password);
  beginRemoteDebug(device_name);
  
  // Wait for debug connections
  while(millis()<30000) {
    ArduinoOTA.handle();
    Debug.handle();
    INTERVAL(1000,millis()) {
      debugI("Initial wait %lu",millis());
      Serial.print(".");
    }
  }
  Serial.println();
  configureBME(bme);
  writeLeftBottom("!!");
  u8g2.sendBuffer();

  // Dim display
  u8x8_cad_StartTransfer(u8g2.getU8x8());
  u8x8_cad_SendCmd( u8g2.getU8x8(), 0xD9);
  u8x8_cad_SendArg( u8g2.getU8x8(), 5);  //max 34
  u8x8_cad_EndTransfer(u8g2.getU8x8());
}

void loop() 
{
  loopstart = millis();

  if (bme.run()) { // If new data is available
    u8g2.setColorIndex(1);
    u8g2.setFont(u8g2_font_7x14_mf);

    sprintf(m_str, " %.2fC", bme.temperature);
    u8g2.drawStr(36,15,m_str); 

    sprintf(m_str, " %.2f%%", bme.humidity);
    u8g2.drawStr(36,31,m_str); 

    if (bme.staticIaq<10) {
      sprintf(m_str, "  %.2f", bme.staticIaq);
    } else if (bme.staticIaq<100) {
      sprintf(m_str, " %.2f", bme.staticIaq);
    } else {
      sprintf(m_str, "%.2f", bme.staticIaq);
    }
    u8g2.drawStr(36,45,m_str); 

    INTERVAL(120000,millis())
    {
      startNetwork(ssid,password,device_name,ip,gateway,subnet,dns1);
      delay(1);
      // pubsubclient.setCallback(mqtt_callback_func);
      mqtt_controller.handle();
      publishBME(pubsubclient,bme,mqtt_topic_base);
      mqtt_controller.handle();
      pubsubclient.disconnect(); 
      espClient.flush();
      // wait until connection is closed completely
      while( pubsubclient.state() != -1){  
          delay(1);
      }
    }
  } else {
    checkBME(bme);
  }
  delay(1);
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  btStop();
  adc_power_off();
  esp_wifi_stop();
  esp_bt_controller_disable();
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);

  sprintf(m_str, "%lu", loopduration);
  writeLeftBottom(m_str); 
  u8g2.sendBuffer();
  loopduration = millis()-loopstart;
  esp_light_sleep_start();
}
