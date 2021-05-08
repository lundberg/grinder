#ifndef __BUTTON__
#define __BUTTON__

#include <Arduino.h>
#include <Input.h>

enum class ButtonEvent : uint8_t {
  UP           = 1 << 0,
  DOWN         = 1 << 1,
  CLICK        = 1 << 2,
  LONG_HOLD    = 1 << 3,
};

class Button : public Input<ButtonEvent> {

public:
  Button(uint8_t pin, uint8_t activeLogicLevel = LOW) :
    Input<ButtonEvent>(pin, activeLogicLevel) {}

  Button(uint8_t pin, ButtonEvent events = ButtonEvent::CLICK, uint8_t activeLogicLevel = LOW, int16_t analogThreshold = 0) :
    Input<ButtonEvent>(pin, events, activeLogicLevel, analogThreshold) {
      this->state = this->inactiveLogicLevel;
    }

  virtual bool tick() override;
  void setHoldDuration(unsigned long ms);

protected:
  uint16_t holdDuration = 1000;
  volatile ButtonEvent lastEvent = static_cast<ButtonEvent>(0);
  volatile uint8_t state;
  volatile uint32_t startTime = 0;
};

#endif