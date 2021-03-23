#include <Arduino.h>

//#define TINY4KOLED_QUICK_BEGIN  // Uncomment for white OLED
#include <TinyWireM.h>
#include <Tiny4kOLED.h>

#include <Button.h>
#include <RotaryEncoder.h>

#include <main.h>
#include <profiles.h>

#define START_PIN PB1

Button startButton(START_PIN, Button::Event::DOWN);
Button menuButton(A0, Button::Event::CLICK | Button::Event::LONG_HOLD, 900);
RotaryEncoder encoder(PB3, PB4, RotaryEncoder::LatchMode::FOUR3);

ISR(PCINT0_vect) {
  // Interrupt handler for rotary encoder
  if (state != State::GRINDING) {
    encoder.tick();
  }
}

void setup() {
  // Attach Rotary Encoder Iterrupts
  cli();                   // Disable interrupts during setup
  PCMSK |= (1 << PCINT3);  // Enable interrupt handler (ISR) for pin 3
  PCMSK |= (1 << PCINT4);  // Enable interrupt handler (ISR) for pin 4
  GIMSK |= (1 << PCIE);    // Enable PCINT interrupt in the general interrupt mask
  sei(); 

  // Initialize Buttons
  menuButton.begin();
  startButton.begin();
  
  // Load or reset Settings
  if (menuButton.peek() == Button::Event::DOWN) {
    Profiles.reset();
  } else {
    Profiles.load();
  }

  // Initialize screen
  oled.begin();
  oled.setFont(FONT8X16);
  oled.switchRenderFrame();
  oled.clear();
  drawProfile();
  oled.on();
}

void loop() {
  startButton.tick();

  // Handle start button click
  if (startButton.read() == Button::Event::DOWN) {
    if (state == State::GRINDING) {
      // Abort
      abortCountdown();
      state = State::MENU;

    } else if (Profiles.current->timer()) {
      // Start profile grinding timer
      Profiles.save();
      startCountdown();
      state = State::GRINDING;

    } else if (Profiles.current->isManual()) {
      // Manual mode -> Just remember it
      Profiles.save();

    } else if (Profiles.current->isEmpty()) {
      // Started on "empty" profile -> Go to manual mode
      Profiles.setProfile(MANUAL_MODE);
      Profiles.save();
      drawProfile();
    }
  }

  switch (state) {
    default:
    case State::MENU:
      menuHandler();
      break;
    case State::EDIT_PROFILE_TYPE:
    case State::EDIT_PROFILE_TIMER_S:
    case State::EDIT_PROFILE_TIMER_MS:
    case State::EDIT_PROFILE_ICON:
      editProfileHandler();
      break;
    case State::GRINDING:
      grindingHandler();
      break;
  }
}

void menuHandler() {
  menuButton.tick();

  // Handle menu button long hold
  if (menuButton.read() == Button::Event::LONG_HOLD && !Profiles.current->isManual()) {
    // Edit profile
    state = State::EDIT_PROFILE_TYPE;
    drawProfile();
  }

  // Handle rotary encoder
  RotaryEncoder::Direction direction = encoder.getDirection();
  if (direction != RotaryEncoder::Direction::NOROTATION) {
    Profiles.changeProfile((int8_t)direction);
    drawProfile();
  }
}

void editProfileHandler() {
  menuButton.tick();

  // Handle menu button click
  Button::Event event = menuButton.read();
  if (event == Button::Event::CLICK && !Profiles.current->isEmpty()) {
    // Cycle profile edit field
    state = (State)((int8_t)state << 1);
    if (state > State::EDIT_PROFILE_ICON) {
      state = State::EDIT_PROFILE_TYPE;
    }
    drawProfile();
    
  } else if (event == Button::Event::LONG_HOLD) {
    // Save profile and go back to menu
    Profiles.save();
    state = State::MENU;
    drawProfile();
  }

  // Handle rotary encoder
  RotaryEncoder::Direction direction = encoder.getDirection();
  if (direction != RotaryEncoder::Direction::NOROTATION) {
    if (state == State::EDIT_PROFILE_TYPE) {
      Profiles.current->changeType((int8_t)direction);
    } else if (state == State::EDIT_PROFILE_TIMER_S) {
      Profiles.current->changeTimer(1000 * (int8_t)direction);
    } else if (state == State::EDIT_PROFILE_TIMER_MS) {
      Profiles.current->changeTimer(100 * (int8_t)direction);
    } else if (state == State::EDIT_PROFILE_ICON) {
      Profiles.current->changeIcon((int8_t)direction);
    }
    drawProfile();
  }
}

void drawProfile() {
  Profile* current = Profiles.current;

  // Profile/Manual Label
  if (current->isManual()) {
    // Manual
    oled.setCursor(0, 1);
  } else {
    // Profile
    oled.setCursor(0, 0);
    oled.print(Profiles.data.current + 1); oled.print('.');
  }
  oled.invertOutput(state == State::EDIT_PROFILE_TYPE);
  oled.print(current->label());

  // Profile Timer
  if (auto timer = current->timer()) {
    // Icon
    oled.invertOutput(false);
    oled.bitmap(0, 2, 13, 4, timer_icon13x16);

    // Seconds
    int8_t s = (int8_t) (timer / 1000);
    int8_t ms = (int8_t) ((timer - s * 1000) / 100);
    oled.setCursor(2 * 8, 2);
    oled.invertOutput(state == State::EDIT_PROFILE_TIMER_S);
    oled.print(s);
    oled.invertOutput(false);
    oled.print('.');
    oled.invertOutput(state == State::EDIT_PROFILE_TIMER_MS);
    oled.print(ms);
    oled.invertOutput(false);
    oled.print(F(" sec"));
  }

  // Profile Icon
  if (state == State::EDIT_PROFILE_ICON) {
    // Draw filled square to make edited icons same width
    oled.invertOutput(true);
    for (uint8_t i = 0; i < 4; i++) {
      oled.setCursor(102, i);
      oled.clearToEOL();
    }
  }
  Icon icon = current->icon();
  if (icon.data != NULL) {
    oled.bitmap(128 - icon.width - icon.offset, icon.page, 128 - icon.offset, 4, icon.data);
  }

  // Prepare for next draw
  oled.switchFrame();
  oled.invertOutput(false);
  oled.clear();
}

void startCountdown() {
  oled.switchDisplayFrame();
  stopTime = millis() + Profiles.current->timer();
}

void abortCountdown() {
  oled.switchRenderFrame();
  oled.clear();
  drawProfile();
}

void grindingHandler() {
  int32_t countdown = stopTime - millis();

  if (countdown > 0) {
    // Grinding
    uint8_t newProgress = countdown * 128 / Profiles.current->timer(); 
    if (newProgress != progress) {
      progress = newProgress;
      oled.setCursor(0, 2);
      oled.fillLength(0x0F, progress);
      oled.clearToEOL();
    }

  } else {
    // Grinding Done! -> Simulate start button down
    pinMode(START_PIN, OUTPUT);
    digitalWrite(START_PIN, LOW);
    
    // Clear any last progress bar
    oled.setCursor(0, 2);
    oled.fillLength(0x00, 8);
    // Show "done" screen
    oled.switchRenderFrame();
    oled.clear();
    oled.setCursor(44, 1);
    oled.print("Done!");
    oled.switchFrame();
    oled.clear();
    delay(1000);

    // Simulate release of start button
    digitalWrite(START_PIN, HIGH);
    pinMode(START_PIN, INPUT_PULLUP);
    
    // Go back to profile menu
    state = State::MENU;
    drawProfile();
  }
}