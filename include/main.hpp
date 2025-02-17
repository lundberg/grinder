#ifndef main_h
#define main_h

#include <Arduino.h>
#include <Trigger.hpp>

#define digit(i) (char)(i+48)

#define SLEEP_TIMEOUT 120000  // 2 min

#define PROGRESSBAR_WIDTH 122  // 128 - 2 * 3 padding
#define ICON_OFFSET_X 98  // 128 - manual icon width
#define TIMER_OFFSET_X 24  // 3 chars, 8 pixels each

// Board pin and port definitions
#if defined(__AVR_ATtiny85__)
#define MOTOR_PIN PB1
#define ROTARY_PIN_A PB3
#define ROTARY_PIN_B PB4
#define ROTARY_BUTTON_PIN A0
#define ROTARY_BUTTON_THRESHOLD 900
#define INTERRUPT_VECT PCINT0_vect
#elif defined(__AVR_ATtinyx14__)
#define MOTOR_PIN PIN_PA2
#define ROTARY_PIN_A PIN_PA5
#define ROTARY_PIN_B PIN_PA4
#define ROTARY_BUTTON_PIN PIN_PA6
#define ROTARY_BUTTON_THRESHOLD 0
#define INTERRUPT_VECT PORTA_PORT_vect
#endif

enum class State : uint8_t {
  MENU                  = 1 << 0,
  EDIT_PROFILE          = 1 << 1,
  EDIT_PROFILE_TIMER_S  = 1 << 2,
  EDIT_PROFILE_TIMER_MS = 1 << 3,
  EDIT_PROFILE_ICON     = 1 << 4,
  PROFILE_GRINDING      = 1 << 5,
  MANUAL_GRINDING       = 1 << 6,

  EDIT = (
    EDIT_PROFILE |
    EDIT_PROFILE_TIMER_S |
    EDIT_PROFILE_TIMER_MS |
    EDIT_PROFILE_ICON
  ),
  GRINDING = PROFILE_GRINDING | MANUAL_GRINDING,
};

State operator|(State a, State b) { return (State)((uint8_t)a | (uint8_t)b); }
State operator&(State a, State b) { return (State)((uint8_t)a & (uint8_t)b); }

State state = State::MENU;

uint32_t sleepTime;
uint32_t stopTime;
uint8_t progress = 0;

void menuLoop();
void editLoop();
void profileGrindingLoop();
void manualGrindingLoop();

void switchToBufferRenderFrame();
void erase();
void drawMenu();
void renderProfileTimer();
void renderProfileIcon();
void renderDone();
void renderProgressbar();

void resetSleepTimer();
void sleep();

#endif
