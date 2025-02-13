#ifndef __INPUT__
#define __INPUT__

#include <Arduino.h>

#define invertLogicLevel(l) l == LOW ? HIGH : LOW

template <typename E>
class Input {

public:
  typedef E Event;

  Input(uint8_t pin, uint8_t activeLogicLevel = HIGH) :
    Input(pin, static_cast<E>(0xFF), activeLogicLevel, 0) {}

  Input(uint8_t pin, E events, uint8_t activeLogicLevel, int16_t analogThreshold) {
    this->pin = pin;
    this->activeLogicLevel = activeLogicLevel;
    this->inactiveLogicLevel = invertLogicLevel(activeLogicLevel);
    this->analogThreshold = analogThreshold;
    this->event = static_cast<E>(0);
    this->watch(events);
  }

  void begin() {
    pinMode(this->pin, this->activeLogicLevel == LOW ? INPUT_PULLUP : INPUT);
  }

  void watch(E events) {
    this->events = events;
  }

  bool watches(E event) {
    return (this->events & event) == event;
  }

  E read(bool tick = false) {
    if (tick) this->tick();
    E event = this->event;
    this->event = static_cast<E>(0);
    return event;
  }

  E peek() {
    return static_cast<E>(this->readState() == this->inactiveLogicLevel ? (1 << 0) : (1 << 1));
  }

  bool is(E event) {
    return this->peek() == event;
  }

  virtual bool tick() {
    E event = this->peek();
    if (event != this->event && this->watches(event)) {
      this->event = event;
      return true;
    }
    return false;
  }

protected:
  uint8_t pin;
  uint8_t activeLogicLevel;
  uint8_t inactiveLogicLevel;
  int16_t analogThreshold;

  E events;
  volatile E event;

  uint8_t readState() {
    if (this->analogThreshold) {
      return analogRead(this->pin) < this->analogThreshold ? LOW : HIGH;
    } else {
      return digitalRead(this->pin);
    }
  }
};

template <class E>
inline E operator|(E a, E b) {
  return (E)((uint8_t)a | (uint8_t)b);
}

template <class E>
inline E operator&(E a, E b) {
  return (E)((uint8_t)a & (uint8_t)b);
}

#endif
