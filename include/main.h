#ifndef main_h
#define main_h

#include <Arduino.h>

enum class State {
  MENU                  = 1 << 0,
  EDIT_PROFILE_TYPE     = 1 << 1,
  EDIT_PROFILE_TIMER_S  = 1 << 2,
  EDIT_PROFILE_TIMER_MS = 1 << 3,
  EDIT_PROFILE_ICON     = 1 << 4,
  GRINDING              = 1 << 5
};

State state = State::MENU;

int32_t stopTime;
uint8_t progress = 0;

void menuHandler();
void editProfileHandler();
void grindingHandler();

void drawProfile();
void startCountdown();
void abortCountdown();

#endif