#pragma once

#include<Arduino.h>
#include<MqttListener.hpp>
#include<MqttController.hpp>
#include <Beeper.hpp>
#include<INTERVAL.h>

using namespace std;

class MqttCommandListener : public MqttListener {
    public:  
    MqttCommandListener(MqttController& mqtt_controller, const char* topic)
        : MqttListener(mqtt_controller, topic) {};
    // virtual void handle(void);
    virtual bool presentMessage(const char *topic,const char *payload);
};