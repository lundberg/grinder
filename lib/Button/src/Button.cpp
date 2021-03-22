#include <Arduino.h>
#include <Button.h>

Button::Button(uint8_t pin, Event events, int16_t analogThreshold) {
  this->pin = pin;
  this->analogThreshold = analogThreshold;
  this->watch(events);
}

void Button::begin() {
  pinMode(this->pin, INPUT_PULLUP);
}

void Button::setHoldDuration(unsigned long ms) {
  this->holdDuration = ms;
  this->startTime = 0;
}

void Button::watch(Event events) {
  this->events = events;
}

bool Button::watches(Event event) {
  return (this->events & event) == event;
}

Button::Event Button::read() {
  Event event = this->event;
  this->event = Event::NONE;
  return event;
}

Button::Event Button::peek() {
  return this->readState() == BUTTON_DIGITAL_DOWN ? Event::DOWN : Event::UP;
}

uint8_t Button::readState() {
  if (this->analogThreshold) {
    return analogRead(this->pin) < this->analogThreshold ? LOW : HIGH;
  } else {
    return digitalRead(this->pin);
  }
}

bool Button::tick() {
  uint8_t state = this->readState();
  Event event = this->event;

  if (state == BUTTON_DIGITAL_DOWN) {
    unsigned long now = millis();

    if (this->state == BUTTON_DIGITAL_UP) {
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

  } else if (this->state == BUTTON_DIGITAL_DOWN) {
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