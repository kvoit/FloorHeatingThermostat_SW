#include<Beeper.hpp>

void Beeper::begin() {
    pinMode(this->pin,OUTPUT);
    digitalWrite(this->pin,LOW);
}

bool Beeper::handle() {
    if (millis()-beep_length > beep_start) {
        digitalWrite(this->pin,LOW);
        return false;
    } else {
        return true;
    }
}

void Beeper::beep(uint16_t beeptime) {
    beep_start = millis();
    beep_length = beeptime;
    digitalWrite(this->pin,HIGH);
}
