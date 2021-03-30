#include <Arduino.h>

//#define TINY4KOLED_QUICK_BEGIN  // Uncomment for white OLED
#include <TinyWireM.h>
#include <Tiny4kOLED.h>

#include <Button.h>
#include <RotaryEncoder.h>

#include <main.h>
#include <profiles.h>

#define START_BUTTON_PIN PB1

Button startButton(START_BUTTON_PIN, Button::Event::DOWN);
Button menuButton(A0, Button::Event::CLICK | Button::Event::LONG_HOLD, 900);
RotaryEncoder encoder(PB3, PB4, RotaryEncoder::LatchMode::FOUR3);

ISR(PCINT0_vect) {
  // Interrupt handler for start button and rotary encoder
  startButton.tick();
  encoder.tick();
}

void setup() {
  // Attach Rotary Encoder Iterrupts
  cli();                   // Disable interrupts during setup
  PCMSK |= (1 << PCINT1);  // Enable interrupt handler (ISR) for pin 1 (start button)
  PCMSK |= (1 << PCINT3);  // Enable interrupt handler (ISR) for pin 3 (encoder A)
  PCMSK |= (1 << PCINT4);  // Enable interrupt handler (ISR) for pin 4 (encoder B)
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
  state = Profiles.current ? State::PROFILE : State::MANUAL;

  // Initialize screen
  oled.begin();
  oled.setDisplayClock(1, 15);
  oled.setInternalIref(true);
  oled.setContrast(0xFF);
  oled.setFont(FONT8X16);
  oled.clear();
  oled.switchRenderFrame();
  oled.clear();
  drawMenu();
  oled.on();
}

void loop() {
  // Handle start button click
  if (startButton.read() == Button::Event::DOWN) {
    if (state == State::GRINDING) {
      // Abort
      abortCountdown();
      state = State::PROFILE;

    } else if (state == State::MANUAL) {
      // Started in manual mode -> Just remember it
      Profiles.save();

    } else {
      if (Profiles.current->timer()) {
        // Start profile grinding timer
        startCountdown();
        state = State::GRINDING;

      } else {
        // Started on "empty" profile -> Go to manual mode
        Profiles.setProfile(NONE);
        state = State::MANUAL;
        drawMenu();
      }

      Profiles.save();
    }
  }

  // Dispatch current state loop
  if ((State::MENU & state) == state) {
    menuLoop();
  } else if ((State::EDIT & state) == state) {
    editLoop();
  } else if (state == State::GRINDING) {
    grindingLoop();
  }
}

void menuLoop() {
  RotaryEncoder::Direction direction = encoder.getDirection();
  if (direction != RotaryEncoder::Direction::NOROTATION) {
    // Rotary encoder menu navigation
    Profiles.changeProfile((int8_t)direction);
    state = Profiles.current ? State::PROFILE : State::MANUAL;
    drawMenu();

  } else if (state == State::PROFILE) {
    // Profile menu-button long hold -> Edit
    menuButton.tick();
    if (menuButton.read() == Button::Event::LONG_HOLD) {
      state = State::EDIT_PROFILE_TYPE;
      drawMenu();
    }
  }
}

void editLoop() {
  menuButton.tick();

  // Handle menu button click
  Button::Event event = menuButton.read();
  if (event == Button::Event::CLICK && !Profiles.current->isEmpty()) {
    // Ensure we're on the right render buffer
    if (state != State::EDIT_PROFILE_TYPE) {
      oled.switchRenderFrame();
    }

    // Cycle profile field to edit
    state = (State)((int8_t)state << 1);
    if (state > State::EDIT_PROFILE_ICON) {
      state = State::EDIT_PROFILE_TYPE;
    }

    drawMenu();

    // Render timer and icon fields fast direct on display buffer
    if (state != State::EDIT_PROFILE_TYPE) {
      oled.switchRenderFrame();
    }
    
  } else if (event == Button::Event::LONG_HOLD) {
    // Save profile and go back to menu
    Profiles.save();

    // Ensure we're on the right render buffer
    if (state != State::EDIT_PROFILE_TYPE) {
      oled.switchRenderFrame();
    }

    state = State::PROFILE;
    drawMenu();
  }

  // Handle rotary encoder -> Change current field value
  RotaryEncoder::Direction direction = encoder.getDirection();
  if (direction != RotaryEncoder::Direction::NOROTATION) {
    if (state == State::EDIT_PROFILE_TYPE) {
      Profiles.current->changeType((int8_t)direction);
      drawMenu();
    } else if (state == State::EDIT_PROFILE_TIMER_S) {
      Profiles.current->changeTimer(1000 * (int8_t)direction);
      renderProfileTimer();
    } else if (state == State::EDIT_PROFILE_TIMER_MS) {
      Profiles.current->changeTimer(100 * (int8_t)direction);
      renderProfileTimer();
    } else if (state == State::EDIT_PROFILE_ICON) {
      Profiles.current->changeIcon((int8_t)direction);
      renderProfileIcon();
    }
  }
}

void erase(uint8_t x, uint8_t y, uint8_t x2, uint8_t y2) {
  if (x2 <= x || y2 <= y) return;
  for (; y < y2; y++) {
    oled.setCursor(x, y);
    oled.fillLength(0x00, x2 - x);
  }
}

void drawMenu() {
  if (state == State::MANUAL) {
    // Manual mode
    oled.clear();
    oled.setCursor(0, 1);
    oled.print("Manual");
    oled.bitmap(ICON_OFFSET_X, 0, 128, 4, MANUAL_ICON.data);

  } else {
    // Profile number
    oled.setCursor(0, 0);
    oled.invertOutput(false);
    oled.print(Profiles.data.profile); oled.print('.');
    
    // Profile type / label
    oled.invertOutput(state == State::EDIT_PROFILE_TYPE);
    oled.print(Profiles.current->label());
    
    // Profile timer
    oled.invertOutput(false);
    if (Profiles.current->isEmpty()) {
      // No timer
      erase(0, 2, TIMER_OFFSET_X + 8 * 8, 4);
    } else {
      // Icon
      oled.bitmap(0, 2, 13, 4, timer_icon13x16);
      erase(13, 2, TIMER_OFFSET_X, 4);  // Erase between icon and time
      // Time
      renderProfileTimer();
      oled.invertOutput(false);
      oled.print(" sec");
    } 

    // Profile Icon
    renderProfileIcon();
  }

  oled.switchFrame();
}

void renderProfileTimer() {
  auto timer = Profiles.current->timer();
  int8_t s = (int8_t) (timer / 1000);
  int8_t ms = (int8_t) ((timer - s * 1000) / 100);

  // Seconds
  oled.setCursor(TIMER_OFFSET_X, 2);
  oled.invertOutput(state == State::EDIT_PROFILE_TIMER_S);
  if (s < 10) oled.print(' ');  // Left pad
  oled.print(s);
  oled.invertOutput(false);
  oled.print('.');

  // Millis
  oled.invertOutput(state == State::EDIT_PROFILE_TIMER_MS);
  oled.print(ms);
}

void renderProfileIcon() {
  const Icon& icon = Profiles.current->icon();
  oled.invertOutput(state == State::EDIT_PROFILE_ICON);

  if (icon.data != NULL) {
    const uint8_t x1 = 128 - icon.width - icon.offset;
    const uint8_t x2 = 128 - icon.offset;

    // Clear any icon margins
    if (icon.page) erase(ICON_OFFSET_X, 0, 128, icon.page);  // Top margin
    erase(ICON_OFFSET_X, icon.page, x1, 4);  // Left margin
    erase(x2, icon.page, 128, 4);  // Right margin

    // Draw icon
    oled.bitmap(x1, icon.page, x2, 4, icon.data);

  } else {
    // Draw empty icon
    erase(ICON_OFFSET_X, 0, 128, 4);
  }
}

void startCountdown() {
  // Initialize countdown timer, i.e. future stop time
  stopTime = millis() + Profiles.current->timer();

  // Draw full progress bar
  oled.clear();
  oled.setCursor(0, 2);
  oled.fillLength(0x0F, 128);
  oled.switchFrame();
  oled.switchRenderFrame();
}

void abortCountdown() {
  oled.switchRenderFrame();
  oled.clear();
  drawMenu();
}

void grindingLoop() {
  int32_t countdown = stopTime - millis();

  if (countdown > 0) {
    // Grinding ...
    uint8_t newProgress = countdown * 128 / Profiles.current->timer(); 
    if (newProgress != progress) {
      progress = newProgress;
      oled.setCursor(progress, 2);
      oled.clearToEOL();
    }

  } else {
    // Done! -> Simulate start button down
    pinMode(START_BUTTON_PIN, OUTPUT);
    digitalWrite(START_BUTTON_PIN, LOW);

    // Clear any trailing progress bar
    oled.setCursor(0, 2);
    oled.fillLength(0x00, 8);

    // Show "done" screen
    oled.switchRenderFrame();
    oled.clear();
    oled.setCursor(44, 1);
    oled.print("Done!");
    oled.switchFrame();
    delay(1000);

    // Go back to profile menu
    oled.clear();
    drawMenu();
    state = State::PROFILE;

    // Simulate release of start button
    digitalWrite(START_BUTTON_PIN, HIGH);
    pinMode(START_BUTTON_PIN, INPUT_PULLUP);
    
    // Reset any interrupt triggered events during grinding
    startButton.read();
    encoder.getDirection();
  }
}