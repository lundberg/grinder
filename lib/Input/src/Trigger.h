#ifndef __STOPWATCH__
#define __STOPWATCH__

#include <Arduino.h>

class Trigger {
public:
    Trigger(uint16_t triggerMillis = 0, bool single = true) {
        this->triggerMillis = triggerMillis;
        this->single = single;
    }

    void start() {
        this->startTime = millis();
        this->triggered = false;
        this->count = 0;
    }

    void stop() {
        this->startTime = 0;
        this->triggered = false;
    }

    bool read(bool tick = false) {
        if (tick) this->tick();
        if (this->triggered) {
            this->triggered = false;
            return true;
        }
        return false;
    }    

    void tick() {
        bool triggered = this->startTime && (millis() >= this->startTime + this->triggerMillis);
        if (triggered && !this->triggered) {
            this->triggered = true;
            this->count++;
            if (!this->single) {
                this->startTime = millis();
            }
        }
    }

    uint16_t count = 0;

private:
    uint32_t startTime = 0;
    uint16_t triggerMillis = 0;
    bool single = true;
    bool triggered = false;
};


#endif