#include <BMEFunctions.h>

char pubchar[20];

extern uint32_t loopstart;

void publishBME(PubSubClient &pubsubclient, Bsec &bme, const char* mqtt_topic_base) {
  char mqtt_topic[strlen(mqtt_topic_base)+12];

  dtostrf(bme.temperature, 1, 2, pubchar);
  strcpy(mqtt_topic,mqtt_topic_base);
  strcat(mqtt_topic,"temp");
  if(!pubsubclient.publish(mqtt_topic, pubchar, true)) {
    Serial.println("Send error");
  }

  dtostrf(bme.humidity, 1, 2, pubchar);
  strcpy(mqtt_topic,mqtt_topic_base);
  strcat(mqtt_topic,"hum");
  if(!pubsubclient.publish(mqtt_topic, pubchar, true)) {
    Serial.println("Send error");
  }
  
  dtostrf(bme.pressure/1000.0, 1, 4, pubchar);
  strcpy(mqtt_topic,mqtt_topic_base);
  strcat(mqtt_topic,"pres");
  if(!pubsubclient.publish(mqtt_topic, pubchar, true)) {
    Serial.println("Send error");
  }

  dtostrf(bme.gasResistance/1000.0, 1, 3, pubchar);
  strcpy(mqtt_topic,mqtt_topic_base);
  strcat(mqtt_topic,"gas");
  if(!pubsubclient.publish(mqtt_topic, pubchar, true)) {
    Serial.println("Send error");
  }

  dtostrf(bme.staticIaq, 1, 3, pubchar);
  strcpy(mqtt_topic,mqtt_topic_base);
  strcat(mqtt_topic,"siaq");
  if(!pubsubclient.publish(mqtt_topic, pubchar, true)) {
    Serial.println("Send error");
  }

  dtostrf(bme.iaq, 1, 3, pubchar);
  strcpy(mqtt_topic,mqtt_topic_base);
  strcat(mqtt_topic,"iaq");
  if(!pubsubclient.publish(mqtt_topic, pubchar, true)) {
    Serial.println("Send error");
  }

  dtostrf(bme.co2Equivalent, 1, 3, pubchar);
  strcpy(mqtt_topic,mqtt_topic_base);
  strcat(mqtt_topic,"co2");
  if(!pubsubclient.publish(mqtt_topic, pubchar, true)) {
    Serial.println("Send error");
  }

  dtostrf(bme.breathVocEquivalent, 1, 3, pubchar);
  strcpy(mqtt_topic,mqtt_topic_base);
  strcat(mqtt_topic,"breathVoc");
  if(!pubsubclient.publish(mqtt_topic, pubchar, true)) {
    Serial.println("Send error");
  }

  itoa(millis()-loopstart, pubchar, 10);
  strcpy(mqtt_topic,mqtt_topic_base);
  strcat(mqtt_topic,"time");
  if(!pubsubclient.publish(mqtt_topic, pubchar, true)) {
    Serial.println("Send error");
  }
}

void configureBME(Bsec &bme, uint8_t i2c) {
  bme.begin(i2c, Wire);
  bme.setTemperatureOffset(MYTEMPERATURE_OFFSET);

  const uint8_t bsec_config_iaq[] = {
    #include "config/generic_33v_3s_28d/bsec_iaq.txt"
  };
  bme.setConfig(bsec_config_iaq);

  bsec_virtual_sensor_t sensorList[10] = {
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
    BSEC_OUTPUT_RAW_PRESSURE,
    BSEC_OUTPUT_RAW_GAS,
    BSEC_OUTPUT_IAQ,
    BSEC_OUTPUT_STATIC_IAQ,
    BSEC_OUTPUT_CO2_EQUIVALENT,
    BSEC_OUTPUT_BREATH_VOC_EQUIVALENT,
    
  };

  bme.updateSubscription(sensorList, 10, BSEC_SAMPLE_RATE_LP);
  
}

void checkBME(Bsec &bme)
{
  if (bme.status != BSEC_OK) {
    if (bme.status < BSEC_OK) {
      Serial.print("BSEC error code : ");
      Serial.println(bme.status);
    } else {
      Serial.print("BSEC warning code : ");
      Serial.println(bme.status);
    }
  }

  if (bme.bme680Status != BME680_OK) {
    if (bme.bme680Status < BME680_OK) {
      Serial.print("BME680 error code : ");
      Serial.println(bme.bme680Status);
    } else {
      Serial.print("BME680 warning code : ");
      Serial.println(bme.bme680Status);
    }
  }
}