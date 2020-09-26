#pragma once

#include <Arduino.h>

class Beeper {
    public:
        Beeper(const uint8_t pin)
            : pin(pin) {
                
            };

        void begin();
        bool handle();
        void beep(uint16_t beeptime);

    protected:
        uint32_t beep_start = 0;
        uint32_t beep_length = 0;
        const uint8_t pin;
};