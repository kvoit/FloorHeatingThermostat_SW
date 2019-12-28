#include<Arduino.h>

#include "bsec.h"
#include <PubSubClient.h>

// Helper functions declarations
void checkIaqSensorStatus(void);
void errLeds(void);

void configureBME(Bsec& bme,uint8_t i2c=0x77);
void publishBME(PubSubClient& pubsubclient, Bsec& bme, const char* mqtt_topic_base);
void checkBME(Bsec &bme);