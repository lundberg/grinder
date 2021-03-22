#ifndef main_h
#define main_h

#include <Arduino.h>

enum class State {
  MENU     = 1 << 0,
  EDIT     = 1 << 1,
  GRINDING = 1 << 2
};

State state = State::MENU;

uint8_t edit = 0;
int32_t stopTime;
uint8_t progress = 0;


void menuHandler();
void editProfileHandler();
void grindingHandler();

void drawProfile(bool preClear = false);
void startCountdown();
void saveAndExitEditMode();

#endif