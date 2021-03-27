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
  startButton.tick();
  if (state != State::GRINDING) {
    encoder.tick();
  }
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
  oled.switchRenderFrame();
  oled.clear();
  drawMenu();
  oled.on();
}

void loop() {
  // Handle start button
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

  // Dispatch state handlers
  if ((State::MENU & state) == state) {
    // Menu
    menuHandler();
    if (state == State::PROFILE) {
      profileHandler();
    }

  } else if ((State::EDIT & state) == state) {
    // Edit
    editHandler();

  } else if (state == State::GRINDING) {
    // Grinding
    grindingHandler();
  }
}

void menuHandler() {
  // Handle rotary encoder -> Navigate menu
  RotaryEncoder::Direction direction = encoder.getDirection();
  if (direction != RotaryEncoder::Direction::NOROTATION) {
    Profiles.changeProfile((int8_t)direction);
    state = Profiles.current ? State::PROFILE : State::MANUAL;
    drawMenu();
  }
}

void profileHandler() {
  menuButton.tick();
  
  if (menuButton.read() == Button::Event::LONG_HOLD) {
    // Edit profile
    state = State::EDIT_PROFILE_TYPE;
    drawMenu();
  }
}

void editHandler() {
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

void drawMenu() {
  if (state == State::MANUAL) {
    // Manual
    oled.clear();
    oled.setCursor(0, 1);
    oled.print("Manual");
    oled.bitmap(
      128 - MANUAL_ICON.width - MANUAL_ICON.offset,
      MANUAL_ICON.page,
      128 - MANUAL_ICON.offset,
      4, MANUAL_ICON.data
    );

  } else {
    // Profile number
    oled.setCursor(0, 0);
    oled.invertOutput(false);
    oled.print(Profiles.data.profile); oled.print('.');
    
    // Profile type / label
    oled.invertOutput(state == State::EDIT_PROFILE_TYPE);
    oled.print(Profiles.current->label());
    
    // Timer
    if (!Profiles.current->isEmpty()) {
      oled.invertOutput(false);
      oled.bitmap(0, 2, 13, 4, timer_icon13x16);
    } 
    renderProfileTimer();

    // Icon
    renderProfileIcon();
  }

  // Display and prepare for next draw
  oled.switchFrame();
  oled.invertOutput(false);
  oled.clear();
}

void renderProfileTimer() {
  oled.setCursor(3 * 8, 2);

  if (auto timer = Profiles.current->timer()) {
    int8_t s = (int8_t) (timer / 1000);
    int8_t ms = (int8_t) ((timer - s * 1000) / 100);

    // Seconds
    oled.invertOutput(state == State::EDIT_PROFILE_TIMER_S);
    if (s < 10) oled.print(' ');  // Left pad
    oled.print(s);
    oled.invertOutput(false);
    oled.print('.');

    // Millis
    oled.invertOutput(state == State::EDIT_PROFILE_TIMER_MS);
    oled.print(ms);
    oled.invertOutput(false);
    oled.print(" sec");
  }
}

void renderProfileIcon() {
  auto icon = Profiles.current->icon();
  uint8_t x1 = 128 - icon.width - icon.offset;
  uint8_t x2 = 128 - icon.offset;

  oled.invertOutput(state == State::EDIT_PROFILE_ICON);

  if (icon.data != NULL) {
    if (state == State::EDIT_PROFILE_ICON) {
      // Clear any margins relative to widest icon
      for (uint8_t y = 0; y < 4; y++) {
        if (x1 > PROFILE_ICON_X) {
          oled.setCursor(PROFILE_ICON_X, y);
          oled.fillLength(0x00, x1 - PROFILE_ICON_X);
        }
        if (icon.offset) {
          oled.setCursor(x2, y);
          oled.fillLength(0x00, icon.offset);
        }
      }
    }
    // Draw icon
    oled.bitmap(x1, icon.page, x2, 4, icon.data);

  } else {
    // Draw empty icon
    for (uint8_t y = 0; y < 4; y++) {
      oled.setCursor(PROFILE_ICON_X, y);
      oled.clearToEOL();
    }
  }
}


void startCountdown() {
  // Initialize countdown
  stopTime = millis() + Profiles.current->timer();
  // Draw full progress bar
  oled.setCursor(0, 2);
  oled.fillLength(0x0F, 128);
  oled.switchFrame();
  oled.clear();
  oled.switchRenderFrame();
}

void abortCountdown() {
  oled.switchRenderFrame();
  oled.clear();
  drawMenu();
}

void grindingHandler() {
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
    pinMode(START_PIN, OUTPUT);
    digitalWrite(START_PIN, LOW);
    startButton.read();  // Clear interrupt triggered event

    // Clear any trailing progress bar
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

    // Go back to profile menu
    drawMenu();
    state = State::PROFILE;

    // Simulate release of start button
    digitalWrite(START_PIN, HIGH);
    pinMode(START_PIN, INPUT_PULLUP);
  }
}