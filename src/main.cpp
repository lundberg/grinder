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
  oled.switchRenderFrame();
  oled.clear();
  drawProfile();
  oled.on();
}

void loop() {
  startButton.tick();

  // Handle start button click
  if (startButton.read() == Button::Event::DOWN) {
    if (Profiles.current->isEmpty()) {
      // Go to manual profile if started on empty profile
      Profiles.setProfile(MANUAL_MODE);
      drawProfile();
    }
    saveAndExitEditMode();

    if (state == State::GRINDING) {
      // Abort
      drawProfile(true);
      state = State::MENU;
    } else if (Profiles.current->timer()) {
      // Start grinding timer
      startCountdown();
      state = State::GRINDING;
    }
  }

  switch (state) {
    default:
    case State::MENU:
      menuHandler();
      break;
    case State::EDIT:
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
    state = State::EDIT;
    edit = 1;
    drawProfile();
  }

  // Handle rotary encoder
  RotaryEncoder::Direction direction = encoder.getDirection();
  if (direction != RotaryEncoder::Direction::NOROTATION) {
    int8_t delta = direction == RotaryEncoder::Direction::CLOCKWISE ? 1 : -1;
    Profiles.changeProfile(delta);
    drawProfile();
  }
}

void editProfileHandler() {
  menuButton.tick();

  // Handle menu button click
  Button::Event event = menuButton.read();
  if (event == Button::Event::CLICK && !Profiles.current->isEmpty()) {
    // Cycle profile edit field
    edit++;
    if (edit > 4) {
      edit = 1;
    }
    drawProfile();
    
  } else if (event == Button::Event::LONG_HOLD) {
    // Save profile and go back to menu
    saveAndExitEditMode();
    drawProfile();
  }

  // Handle rotary encoder
  RotaryEncoder::Direction direction = encoder.getDirection();
  if (direction != RotaryEncoder::Direction::NOROTATION) {
    int8_t delta = direction == RotaryEncoder::Direction::CLOCKWISE ? 1 : -1;
    if (edit == 1) {
      Profiles.current->changeType(delta);
    } else if (edit == 2) {
      Profiles.current->changeTimer(1000 * delta);
    } else if (edit == 3) {
      Profiles.current->changeTimer(100 * delta);
    } else if (edit == 4) {
      Profiles.current->changeIcon(delta);
    }
    drawProfile();
  }
}

void drawProfile(bool preClear) {
  Profile* current = Profiles.current;

  if (preClear) {
    oled.switchRenderFrame();
    oled.invertOutput(false);
    oled.clear();
  }
  oled.invertOutput(false);
  oled.setFont(FONT8X16);

  // Profile Label
  if (current->isManual()) {
    // Manual
    oled.setCursor(0, 1);
  } else {
    // Profile
    oled.setCursor(0, 0);
    oled.print(Profiles.data.current + 1); oled.print('.');
    oled.invertOutput(edit == 1);
  }
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
    oled.invertOutput(edit == 2);
    oled.print(s);
    oled.invertOutput(false);
    oled.print('.');
    oled.invertOutput(edit == 3);
    oled.print(ms);
    oled.invertOutput(false);
    oled.print(F(" sec"));
  }

  // Profile Icon
  if (edit == 4) {
    // Empty filled square in edit mode
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
    
    oled.setCursor(0, 2);
    oled.clearToEOL();
    oled.switchRenderFrame();
    oled.clear();
    oled.setCursor(44, 1);
    oled.print("Done!");
    oled.switchFrame();
    oled.clear();
    delay(1000);

    // Release start button
    digitalWrite(START_PIN, HIGH);
    pinMode(START_PIN, INPUT_PULLUP);
    
    // Go back to profile menu
    state = State::MENU;
    drawProfile();
  }
}

void saveAndExitEditMode() {
  Profiles.save();
  if (state == State::EDIT) {
    state = State::MENU;
    edit = 0;
  }
}