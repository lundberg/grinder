#include <Arduino.h>
#include <Button.h>

void Button::setHoldDuration(unsigned long ms) {
  this->holdDuration = ms;
  this->startTime = 0;
}

bool Button::tick() {
  uint8_t state = this->readState();
  Event event = this->event;

  if (state == this->activeLogicLevel) {
    unsigned long now = millis();

    if (this->state == this->inactiveLogicLevel) {
      // Button Down | State: UP -> DOWN
      this->startTime = now;
      event = Event::DOWN;

    } else if (this->watches(Event::LONG_HOLD) &&
               this->startTime &&
               now - this->startTime >= this->holdDuration) {
      // Button Long Click | DOWN -> DOWN
      this->startTime = now;
      event = Event::LONG_HOLD;
    }

  } else if (this->state == this->activeLogicLevel) {
    // Button Up | State: DOWN -> UP
    this->startTime = 0;
    event = Event::UP;

    if (this->watches(Event::CLICK) && this->lastEvent == Event::DOWN) {
      // Button click
      event = Event::CLICK;
    }
  }

  this->state = state;

  if (event != this->event) {
    this->lastEvent = event;

    // Check if event is among watched events
    if (this->watches(event)) {
      this->event = event;
      return true;
    }
  }

  return false;
}