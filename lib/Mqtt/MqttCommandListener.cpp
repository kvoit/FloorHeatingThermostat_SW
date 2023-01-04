#include <MqttCommandListener.hpp>
#include <RemoteDebug.h>

extern boolean pressure_warning;
extern Beeper beeper;
extern RemoteDebug Debug;

bool MqttCommandListener::presentMessage(const char *topic,const char *payload) {
    debugD("Received %s (%s)",topic,payload);
    if(!strncmp(topic,this->getMQTTTopic(),baselength)) {
        if(!strcmp(&topic[baselength],"/pressure_warning/set")) {
            char buffer[64]{};
            strncpy(buffer,topic,(63<baselength)?63:baselength);
            strncat(buffer,"/pressure_warning",63);
            
            if ( !strcmp(payload, "1") ) {
                pressure_warning = true;
                mqtt_controller.sendMessage(buffer,"1");
                return true;
            } else if ( !strcmp(payload, "0") ) {
                pressure_warning = false;
                mqtt_controller.sendMessage(buffer,"0");
                return true;
            }
        }
    } else if(!strncmp(topic,this->getMQTTTopic(),baselength)) {
        if(!strcmp(&topic[baselength],"/beeper/set")) {
            beeper.beep(atoi(payload));
            return true;
        }
    } else {
        debugD("No action for %s (%s)",topic,&topic[baselength]);
    }
    return false;
}