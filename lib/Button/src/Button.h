#ifndef __BUTTON__
#define __BUTTON__

#include <Arduino.h>

#define BUTTON_DIGITAL_UP HIGH
#define BUTTON_DIGITAL_DOWN LOW

class Button {

public:
  enum class Event : uint8_t {
    NONE         = 0,
    UP           = 1 << 0,
    DOWN         = 1 << 1,
    CLICK        = 1 << 2,
    LONG_HOLD    = 1 << 3,
    ALL = UP | DOWN | CLICK | LONG_HOLD,
  };

  Button(uint8_t pin, Event events = Event::CLICK, int16_t analogThreshold = 0);

  void begin();
  void watch(Event events);
  bool watches(Event event);
  void setHoldDuration(unsigned long ms);

  Event peek();
  Event read();
  bool tick();

private:
  uint8_t pin;
  int16_t analogThreshold;
  uint16_t holdDuration = 1000;

  Event events;
  volatile Event event = Event::NONE;
  volatile Event lastEvent = Event::NONE;

  volatile uint8_t state = BUTTON_DIGITAL_UP;
  volatile uint32_t startTime = 0;

  uint8_t readState();
};

inline Button::Event operator|(Button::Event a, Button::Event b) {
  return (Button::Event)((uint8_t)a | (uint8_t)b);
}

inline Button::Event operator&(Button::Event a, Button::Event b) {
  return (Button::Event)((uint8_t)a & (uint8_t)b);
}

#endif