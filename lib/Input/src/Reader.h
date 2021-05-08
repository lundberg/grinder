#ifndef __READER__
#define __READER__

#include <Arduino.h>
#include <Input.h>

enum class ReaderEvent : uint8_t {
  INACTIVE = 1 << 0,
  ACTIVE   = 1 << 1,
};

class Reader : public Input<ReaderEvent> {
public:
  Reader(uint8_t pin, uint8_t activeLogicLevel = HIGH) :
    Input<ReaderEvent>(pin, activeLogicLevel) {}

  Reader(uint8_t pin, ReaderEvent events, uint8_t activeLogicLevel = HIGH, int16_t analogThreshold = 0) :
    Input<ReaderEvent>(pin, events, activeLogicLevel, analogThreshold) {}
};

#endif