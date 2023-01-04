#pragma once

#include <BaseConfigDef.h>

const char *ssid = "";
const char *password = "";

const char *ota_password = OTAPW;

const char *device_name     = "therm" MYDEVICE;
const char *mqtt_server     = "";
const char *mqtt_user       = "";
const char *mqtt_pw         = "";

const char *mqtt_topic_base = "myprefix/" MYTOPIC "/thermostat/";