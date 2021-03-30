#ifndef main_h
#define main_h

#include <Arduino.h>

#define digit(i) (char)(i+48)

enum class State : uint8_t {
  MANUAL                = 1 << 0,
  PROFILE               = 1 << 1,
  EDIT_PROFILE_TYPE     = 1 << 2,
  EDIT_PROFILE_TIMER_S  = 1 << 3,
  EDIT_PROFILE_TIMER_MS = 1 << 4,
  EDIT_PROFILE_ICON     = 1 << 5,
  GRINDING              = 1 << 6,

  MENU = MANUAL | PROFILE, 
  EDIT = (
    EDIT_PROFILE_TYPE |
    EDIT_PROFILE_TIMER_S |
    EDIT_PROFILE_TIMER_MS |
    EDIT_PROFILE_ICON
  ),
};

State operator|(State a, State b) { return (State)((uint8_t)a | (uint8_t)b); }
State operator&(State a, State b) { return (State)((uint8_t)a & (uint8_t)b); }

State state = State::MANUAL;

uint32_t stopTime;
uint8_t progress = 0;

void menuLoop();
void editLoop();
void grindingLoop();

void switchToBufferRenderFrame();
void erase();
void drawMenu();
void renderProfileTimer();
void renderProfileIcon();

void startCountdown();
void abortCountdown();

#endif