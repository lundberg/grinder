#include <Arduino.h>
#include <avr/sleep.h>

#define TINY4KOLED_QUICK_BEGIN  // White OLED
#if defined(__AVR_ATtiny85__)
#include <TinyWireM.h>
#endif
#include <Tiny4kOLED.h>
#include <font8x16atari.h>

#include <Reader.h>
#include <Button.h>
#include <Trigger.h>
#include <RotaryEncoder.h>

#include <main.h>
#include <profiles.h>

Reader motorInput(MOTOR_PIN);
Button menuButton(ROTARY_BUTTON_PIN, Button::Event::CLICK | Button::Event::LONG_HOLD, LOW, ROTARY_BUTTON_THRESHOLD);
RotaryEncoder encoder(ROTARY_PIN_A, ROTARY_PIN_B, RotaryEncoder::LatchMode::FOUR3);

Trigger frameCounter(5, false);

#if defined(__AVR_ATtiny85__)
ISR(PCINT0_vect) {
  /**
  * Interrupt routine for grinder motor and rotary encoder
  **/
  motorInput.tick();
  encoder.tick();
}
#elif defined(__AVR_ATtinyx14__)
ISR(PORTA_PORT_vect) {
  /**
  * Interrupt routine for grinder motor and rotary encoder
  **/
  // Clear interrupt flag to stop ISR from continously retriggering
  // https://github.com/SpenceKonde/megaTinyCore/blob/master/megaavr/extras/PinInterrupts.md#the-isr
  byte flags = PORTA.INTFLAGS;
  PORTA.INTFLAGS = flags;  

  if (flags & (PIN4_bm | PIN5_bm)) {
    // One of rotary encoder pins state changed
    encoder.tick();
  } else if (flags & PIN2_bm) {
    // Motor pin state changed
    motorInput.tick();
  }
}
#endif

void setup() {
  // Initialize inputs
  menuButton.begin();
  motorInput.begin();

  // Attach Iterrupt Service Routines
  #if defined(__AVR_ATtiny85__)
    // Enable Interrupt S(ervice) R(outines) for pin change
    cli();                   // Disable interrupts during setup
    PCMSK |= (1 << PCINT1);  // Enable interrupt handler (ISR) for pin PB1 (motor)
    PCMSK |= (1 << PCINT3);  // Enable interrupt handler (ISR) for pin PB3 (encoder A)
    PCMSK |= (1 << PCINT4);  // Enable interrupt handler (ISR) for pin PB4 (encoder B)
    GIMSK |= (1 << PCIE);    // Enable PCINT interrupt in the general interrupt mask
    sei();                   // Enable interrupts
  #elif defined(__AVR_ATtinyx14__)
    // Enable Interrupt S(ervice) R(outines) for I(nterrupt) S(ense) C(ontrol)
    // https://github.com/SpenceKonde/megaTinyCore/blob/master/megaavr/extras/PinInterrupts.md#enabling-the-interrupt
    PORTA.PIN2CTRL |= (1 << 0);  // Enable ISC [on change] to ISR for pin [PA2] (motor)
    PORTA.PIN4CTRL |= (1 << 0);  // Enable ISC [on change] to ISR for pin [PA4] (encoder A)
    PORTA.PIN5CTRL |= (1 << 0);  // Enable ISC [on change] to ISR for pin [PA5] (encoder B)
  #endif

  // Load or reset profile settings
  uint8_t profilesResetCause = 0;
  delay(44);  // Wait for debounce capacitor to be charged

  if (menuButton.is(Button::Event::DOWN)) {
    // Manual settings reset
    Profiles.reset();
    profilesResetCause = 1;
  } else if (!Profiles.load()) {
    // Forced settings reset due to corrupt or non-existant settings
    profilesResetCause = 2;
  }

  // Initialize display
  oled.begin();
  oled.setDisplayClock(1, 0);
  oled.setInternalIref(true);
  oled.setContrast(0x60);
  oled.setFont(FONT8X16ATARI);
  oled.clear();
  oled.switchRenderFrame();
  oled.clear();
  
  // Show settings reset cause
  if (profilesResetCause) {
    oled.setCursor(0, 1);
    oled.print(profilesResetCause == 1 ? " Settings Reset" : "Corrupt Settings");
    oled.switchFrame();
    oled.on();
    delay(3000);
  }

  // Render display
  drawMenu();
  oled.clear();
  oled.on();

  // Set future sleep time
  resetSleepTimer();
}

void loop() {
  // Handle grinder motor starting and stopping
  Reader::Event motorEvent = motorInput.read();
  if (motorEvent == Reader::Event::ACTIVE) {
    // Prepare display frame for fast and direct rendring
    switchToBufferRenderFrame();
    oled.invertOutput(false);
    oled.clear();
    oled.switchFrame();
    oled.switchRenderFrame();

    if (state == State::PROFILE_GRINDING) {
      // Started grinding profile
      // Initialize countdown timer, i.e. future stop time. TODO: Use Trigger class
      stopTime = millis() + Profiles.current->timer() + COUNTDOWN_UNSIGNED_OFFSET;

    } else {
      // Started manual grinding timer 
      frameCounter.start();
      state = State::MANUAL_GRINDING;
      if (Profiles.current) {
        Profiles.setProfile(_NONE);
      }
    }
    Profiles.save();

  } else if (motorEvent == Reader::Event::INACTIVE) {
    // Grinder motor stopped, i.e. done with either profile or manual
    frameCounter.stop();
    renderDone();
    delay(1500);

    // Go back to menu
    state = State::MENU;
    oled.clear();
    drawMenu();
  }

  // Dispatch current state loop
  if (state == State::MENU) {
    menuLoop();
  } else if ((state & State::EDIT) == state) {
    editLoop();
  } else if (state == State::PROFILE_GRINDING) {
    profileGrindingLoop();
  } else if (state == State::MANUAL_GRINDING) {
    manualGrindingLoop();
  }
}

void menuLoop() {
  // Handle rotary encoder -> Menu navigation
  RotaryEncoder::Direction direction = encoder.getDirection();
  if (direction != RotaryEncoder::Direction::NOROTATION) {
    Profiles.changeProfile((int8_t)direction);
    drawMenu();
  } 

  // Handle menu button
  Button::Event event = menuButton.read(true);
  if (event == Button::Event::CLICK) {
    if (Profiles.current && Profiles.current->timer()) {
      // Start grinding profile and grinder motor
      pinMode(MOTOR_PIN, OUTPUT);
      digitalWrite(MOTOR_PIN, HIGH);
      state = State::PROFILE_GRINDING;
    }

  } else if (event == Button::Event::LONG_HOLD) {
    if (Profiles.current) {
      // Enter edit mode
      state = State::EDIT_PROFILE;
      drawMenu();

    } else {
      // Show version
      oled.switchRenderFrame();
      oled.setCursor(56, 1);
      oled.print(digit(VERSION.major));
      oled.print('.');
      oled.print(digit(VERSION.minor));
    }

  // Check if it's time to go to sleep
  } else if (millis() > sleepTime) {
    sleep();
  }
}

void editLoop() {
  // Handle menu button
  Button::Event event = menuButton.read(true);
  if (event == Button::Event::CLICK && !Profiles.current->isEmpty()) {
    // Click -> Cycle profile field to edit
    state = (State)((int8_t)state << 1);
    if (state > State::EDIT_PROFILE_ICON) {
      state = State::EDIT_PROFILE;
    }
    drawMenu();
    // Render timer and icon fields fast, direct on display frame
    if (state != State::EDIT_PROFILE) {
      oled.switchRenderFrame();
    }
    
  } else if (event == Button::Event::LONG_HOLD) {
    // Long hold -> Save profile and leave edit mode
    Profiles.save();
    state = State::MENU;
    drawMenu();
  }

  // Handle rotary encoder -> Change current field value
  RotaryEncoder::Direction direction = encoder.getDirection();
  if (direction != RotaryEncoder::Direction::NOROTATION) {
    if (state == State::EDIT_PROFILE) {
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

void switchToBufferRenderFrame() {
  if (oled.currentRenderFrame() == oled.currentDisplayFrame()) {
    oled.switchRenderFrame();
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
  resetSleepTimer();
  switchToBufferRenderFrame();

  if (!Profiles.current) {
    // Draw manual mode
    oled.clear();
    oled.setCursor(0, 1);
    oled.print("Manual");
    #if defined(__AVR_ATtinyx14__)
      oled.bitmap(ICON_OFFSET_X, 0, 128, 4, MANUAL_ICON.data);
    #endif

  } else {
    // Draw profile number
    oled.setCursor(0, 0);
    oled.invertOutput(false);
    oled.print(digit(Profiles.data.profile)); oled.print('.');
    
    // Draw profile type / label
    oled.invertOutput(state == State::EDIT_PROFILE);
    oled.print(Profiles.current->label());
    
    // Draw profile timer
    oled.invertOutput(false);
    if (Profiles.current->isEmpty()) {
      // No timer/profile set
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

    // Draw profile Icon
    renderProfileIcon();
  }

  oled.switchFrame();
}

void renderProfileTimer() {
  auto timer = Profiles.current->timer();
  uint8_t s = (uint8_t) (timer / 1000);
  uint8_t ms = (uint8_t) ((timer - s * 1000) / 100);

  // Seconds
  oled.setCursor(TIMER_OFFSET_X, 2);
  oled.invertOutput(state == State::EDIT_PROFILE_TIMER_S);
  if (s < 10) oled.print(' ');  // Left pad
  oled.print(s);
  oled.invertOutput(false);
  oled.print('.');

  // Millis
  oled.invertOutput(state == State::EDIT_PROFILE_TIMER_MS);
  oled.print(digit(ms));
}

void renderProfileIcon() {
  const Icon& icon = Profiles.current->icon();
  oled.invertOutput(state == State::EDIT_PROFILE_ICON);

  if (icon.data != NULL) {
    const uint8_t x1 = 128 - icon.width - icon.offset;
    const uint8_t x2 = 128 - icon.offset;

    // Clear any icon margins
    if (icon.page) erase(ICON_OFFSET_X, 0, 128, icon.page);  // Top margin
    erase(ICON_OFFSET_X, icon.page, x1, 4);                  // Left margin
    erase(x2, icon.page, 128, 4);                            // Right margin

    // Draw icon
    oled.bitmap(x1, icon.page, x2, 4, icon.data);

  } else {
    // Draw empty icon
    erase(ICON_OFFSET_X, 0, 128, 4);
  }
}

void renderDone() {
  // Show "Done" screen
  switchToBufferRenderFrame();
  oled.clear();
  #if defined(__AVR_ATtinyx14__)
    oled.bitmap(28, 1, 49, 3, cup_icon21x16);
  #endif
  oled.setCursor(58, 1);
  oled.print("Done!");
  oled.switchFrame();
  oled.on();
  delay(1500);

  // Go back to menu
  state = State::MENU;
  oled.clear();
  drawMenu();
}

void profileGrindingLoop() {
  uint32_t countdown = stopTime - millis();

  if (countdown > COUNTDOWN_UNSIGNED_OFFSET && menuButton.is(Button::Event::UP)) {
    // Grinding ...
    countdown -= COUNTDOWN_UNSIGNED_OFFSET;  // Remove unsigned long offset
    countdown *= 126;  // Scale to screen width
    uint8_t newProgress = countdown / Profiles.current->timer(); 
    if (newProgress != progress) {
      oled.fillLength(0x00, 2);  // Clear old dot
      progress = newProgress;
      oled.setCursor(progress, 2);
      oled.fillLength(0b00000011, 2);  // Draw new dot
    }

  } else {
    // Done! -> Stop grinder motor
    digitalWrite(MOTOR_PIN, LOW);
    motorInput.begin();

    // Reset any interrupt triggered events while grinding
    encoder.getDirection();
  }
}

void manualGrindingLoop() {
  if (frameCounter.read(true)) {
    uint8_t x = 127 - frameCounter.count;
    oled.setCursor(x == 126 ? 0 : x + 1, 2);
    oled.fillLength(0x00, 2);  // Clear old dot
    oled.setCursor(x, 2);
    oled.fillLength(0b00000011, 2);  // Draw new dot
    if (x == 0) {
      frameCounter.count = 0;  // Loop
    }
  }
}

void resetSleepTimer() {
  sleepTime = millis() + SLEEP_TIMEOUT;
}

void sleep() {
  /**
   * ATtiny85
   * http://www.technoblogy.com/show?2RA3
   * 
   * ATtiny1614
   * https://github.com/SpenceKonde/megaTinyCore/blob/master/megaavr/extras/PowerSave.md
   **/
  oled.off();

  // Go to sleep
  #if defined(__AVR_ATtiny85__)
    ADCSRA &= ~(1 << ADEN);  // Disable ADC
  #endif
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  sleep_cpu();

  // --- sleeping --

  // Sleep interrupted -> Wake up
  sleep_disable();
  #if defined(__AVR_ATtiny85__)
    ADCSRA |= (1 << ADEN);  // Enable ADC
  #endif

  oled.on();
  resetSleepTimer();
}