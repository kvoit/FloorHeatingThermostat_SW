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
#include <MqttCommandListener.hpp>
#include <Beeper.hpp>

#include <BMEFunctions.h>
#include <NetworkFunctions.h>
#include <OTAFunctions.h>
#include <MqttFunctions.h>
#include <RemoteDebugFunctions.h>

#include <ArduinoJson.h>

void sendMQTTDiscoveryMsg();

#define uS_TO_S_FACTOR 1000000  
#define TIME_TO_SLEEP  1    

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE); //, /* clock=*/ 16, /* data=*/ 17);   // ESP32 Thing, HW I2C with pin remapping
const uint32_t chipID = ESP.getEfuseMac();

RemoteDebug Debug;

WiFiClient espClient;

PubSubClient pubsubclient(mqtt_server, 1883, espClient);
MqttController mqtt_controller(pubsubclient,device_name,mqtt_user,mqtt_pw);
MqttCommandListener commandListener(mqtt_controller,mqtt_topic_base);
void mqtt_callback_func(const char* topic, const byte* payload, unsigned int length) { mqtt_controller.callback(topic, payload,length); }

Bsec bme;

Beeper beeper(13);

uint32_t loopstart;
uint32_t loopduration = 0;
char m_str[11] = {};

boolean pressure_warning = false;

uint8_t pressurehist_pointer = 0;
const uint8_t pressurehist_LEN = 15;
const uint8_t pressurehist_DROP = 40; //Pa
float pressurehist[pressurehist_LEN] = {200000.0};

IPAddress ip( 10,166,70, MYLOWIP );
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
  // adc_power_release();

  Serial.begin(115200);	
  Wire.setClock(100000);
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
  
  // pinMode(13,OUTPUT);
  // digitalWrite(13,HIGH);
  // delay(1000);
  // digitalWrite(13,LOW);

  // Wait for debug connections
  while(millis()<30000) {
    ArduinoOTA.handle();
    Debug.handle();
    INTERVAL(1000,millis()) {
      debugI("Initial wait %lu, %s",millis(),WiFi.macAddress().c_str());
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
  u8x8_cad_SendArg( u8g2.getU8x8(), MYDISPLAY_BRIGHTNESS);  //max 34
  u8x8_cad_EndTransfer(u8g2.getU8x8());

  beeper.begin();
  #if BEEPER_START == 1
    beeper.beep(1000);
  #endif
  while(beeper.handle()) {delay(10);};

  sendMQTTDiscoveryMsg();
}

void loop() 
{
  // loopstart = millis();

  ArduinoOTA.handle();
  Debug.handle();
  mqtt_controller.handle();
  beeper.handle();

  INTERVAL(100,millis()) {
    if (bme.run()) { // If new data is available
      
      u8g2.setColorIndex(1);
      u8g2.setFont(u8g2_font_7x14_mf);

      sprintf(m_str, " %.2fC", bme.temperature);
      u8g2.drawStr(36,15,m_str); 
      
      sprintf(m_str, " %.2f%%", bme.humidity);
      u8g2.drawStr(36,29,m_str); 
      
      if (bme.staticIaq<10) {
        sprintf(m_str, "  %.2f", bme.staticIaq);
      } else if (bme.staticIaq<100) {
        sprintf(m_str, " %.2f", bme.staticIaq);
      } else {
        sprintf(m_str, "%.2f", bme.staticIaq);
      }
      u8g2.drawStr(36,43,m_str); 

      sprintf(m_str, " %.0fPa", bme.pressure);
      u8g2.drawStr(30,57,m_str); 

      if(pressure_warning)
      {
        if(bme.pressure < pressurehist[(pressurehist_pointer+1)%pressurehist_LEN]-pressurehist_DROP) {
          beeper.beep(500);
        }
        debugV("%01d: %.0f - %01d: %.0f", pressurehist_pointer, bme.pressure , (pressurehist_pointer+1)%pressurehist_LEN, pressurehist[(pressurehist_pointer+1)%pressurehist_LEN]);
        pressurehist[pressurehist_pointer] = bme.pressure;
        pressurehist_pointer = (pressurehist_pointer+1)%pressurehist_LEN;
      }

      INTERVAL(120000,millis())
      {
        // startNetwork(ssid,password,device_name,ip,gateway,subnet,dns1);
        // delay(1);
        // pubsubclient.setCallback(mqtt_callback_func);
        mqtt_controller.handle();
        publishBME(pubsubclient,bme,mqtt_topic_base);
        mqtt_controller.handle();
        // pubsubclient.disconnect(); 
        // espClient.flush();
        // // wait until connection is closed completely
        // while( pubsubclient.state() != -1){  
        //     delay(1);
        // }
      }
    } else {
      // checkBME(bme);
    }
  }
  // delay(1);
  // WiFi.disconnect(true);
  // WiFi.mode(WIFI_OFF);
  // btStop();
  // adc_power_off();
  // esp_wifi_stop();
  // esp_bt_controller_disable();
  // esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  INTERVAL(1000,millis()) {
    sprintf(m_str, "%u", loopduration);
    writeLeftBottom(m_str); 
    u8g2.sendBuffer();
  }

  // loopduration = millis()-loopstart;
  // esp_light_sleep_start();
  INTERVAL(1000,millis()) {
    if(WiFi.status() != WL_CONNECTED) {
      WiFi.reconnect();
    }
  } 
}

void sendMQTTDiscoveryMsg() {
    DynamicJsonDocument doc(1024);
    size_t n;

    char discovery_msg[1024]{};  
    char name_buffer[64]{};
    char device_buffer[64]{};
    char unique_id_buffer[64]{};

    char discovery_topic[128]{};
    char mode_state_topic[128]{};

    snprintf(device_buffer, sizeof(name_buffer), "thermostate_%x", chipID);

    mqtt_controller.handle();
    delay(100);
    mqtt_controller.handle();
    delay(100);

    // iaq sensor
    doc.clear();

    snprintf(name_buffer, sizeof(name_buffer), "IAQ (%s)", MYTOPIC);
    snprintf(unique_id_buffer, sizeof(name_buffer), "iaq_%s_%x)", MYTOPIC, chipID);
    snprintf(discovery_topic, sizeof(discovery_topic), "homeassistant/sensor/thermostate/sensor_%s_iaq/config", MYTOPIC);
    snprintf(mode_state_topic, sizeof(mode_state_topic), "%siaq", mqtt_topic_base);

    doc["name"] = name_buffer;
    doc["unique_id"] = unique_id_buffer;
    doc["state_topic"] = mode_state_topic;
    doc["unit_of_measurement"] = "iaq";
    doc["device"]["name"] = device_buffer;
    doc["device"]["identifiers"][0] = chipID;

    n = serializeJson(doc, discovery_msg);   
    mqtt_controller.handle();
    mqtt_controller.sendMessage(discovery_topic, discovery_msg, true);
    delay(50);
    mqtt_controller.handle();

    // temp sensor
    doc.clear();

    snprintf(name_buffer, sizeof(name_buffer), "Temperature (%s)", MYTOPIC);
    snprintf(unique_id_buffer, sizeof(name_buffer), "temperature_%s_%x)", MYTOPIC, chipID);
    snprintf(discovery_topic, sizeof(discovery_topic), "homeassistant/sensor/thermostate/sensor_%s_temperature/config", MYTOPIC);
    snprintf(mode_state_topic, sizeof(mode_state_topic), "%stemp", mqtt_topic_base);

    doc["name"] = name_buffer;
    doc["unique_id"] = unique_id_buffer;
    doc["state_topic"] = mode_state_topic;
    doc["unit_of_measurement"] = "°C";
    doc["device_class"] = "temperature";
    doc["device"]["name"] = device_buffer;
    doc["device"]["identifiers"][0] = chipID;

    n = serializeJson(doc, discovery_msg);   
    mqtt_controller.handle();
    mqtt_controller.sendMessage(discovery_topic, discovery_msg, true);
    delay(50);
    mqtt_controller.handle();

    // humidity sensor
    doc.clear();

    snprintf(name_buffer, sizeof(name_buffer), "Humidity (%s)", MYTOPIC);
    snprintf(unique_id_buffer, sizeof(name_buffer), "humidity_%s_%x)", MYTOPIC, chipID);
    snprintf(discovery_topic, sizeof(discovery_topic), "homeassistant/sensor/thermostate/sensor_%s_humidity/config", MYTOPIC);
    snprintf(mode_state_topic, sizeof(mode_state_topic), "%shum", mqtt_topic_base);

    doc["name"] = name_buffer;
    doc["unique_id"] = unique_id_buffer;
    doc["state_topic"] = mode_state_topic;
    doc["unit_of_measurement"] = "%";
    doc["device_class"] = "humidity";
    doc["device"]["name"] = device_buffer;
    doc["device"]["identifiers"][0] = chipID;

    n = serializeJson(doc, discovery_msg);   
    mqtt_controller.handle();
    mqtt_controller.sendMessage(discovery_topic, discovery_msg, true);
    delay(50);
    mqtt_controller.handle();

    // pressure sensor
    doc.clear();

    snprintf(name_buffer, sizeof(name_buffer), "Pressure (%s)", MYTOPIC);
    snprintf(unique_id_buffer, sizeof(name_buffer), "pressure_%s_%x)", MYTOPIC, chipID);
    snprintf(discovery_topic, sizeof(discovery_topic), "homeassistant/sensor/thermostate/sensor_%s_pressure/config", MYTOPIC);
    snprintf(mode_state_topic, sizeof(mode_state_topic), "%spres", mqtt_topic_base);

    doc["name"] = name_buffer;
    doc["unique_id"] = unique_id_buffer;
    doc["state_topic"] = mode_state_topic;
    doc["unit_of_measurement"] = "kPa";
    doc["device_class"] = "pressure";
    doc["device"]["name"] = device_buffer;
    doc["device"]["identifiers"][0] = chipID;

    n = serializeJson(doc, discovery_msg);   
    mqtt_controller.handle();
    mqtt_controller.sendMessage(discovery_topic, discovery_msg, true);
    delay(50);
    mqtt_controller.handle();

    // siaq sensor
    doc.clear();

    snprintf(name_buffer, sizeof(name_buffer), "SIAQ (%s)", MYTOPIC);
    snprintf(unique_id_buffer, sizeof(name_buffer), "siaq_%s_%x)", MYTOPIC, chipID);
    snprintf(discovery_topic, sizeof(discovery_topic), "homeassistant/sensor/thermostate/sensor_%s_siaq/config", MYTOPIC);
    snprintf(mode_state_topic, sizeof(mode_state_topic), "%ssiaq", mqtt_topic_base);

    doc["name"] = name_buffer;
    doc["unique_id"] = unique_id_buffer;
    doc["state_topic"] = mode_state_topic;
    doc["unit_of_measurement"] = "siaq";
    doc["device"]["name"] = device_buffer;
    doc["device"]["identifiers"][0] = chipID;

    n = serializeJson(doc, discovery_msg);   
    mqtt_controller.handle();
    mqtt_controller.sendMessage(discovery_topic, discovery_msg, true);
    delay(50);
    mqtt_controller.handle();
}