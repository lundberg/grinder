#ifndef __SETTINGS__
#define __SETTINGS__

#include <Arduino.h>
#include <EEPROM.h>

#include <icons.h>

#define OFF -1
#define NONE 0
#define EMPTY 0

#define MAX_PROFILE_COUNT 9
#define PROFILE_TYPE_COUNT 3

#define PROFILE_ICON_COUNT 3
#define ICON_OFFSET_X 98  // 128 - manual icon width
#define TIMER_OFFSET_X 24  // 3 chars, 8 pixels each

#define MIN_TIMER 1000
#define MAX_TIMER 30000

int16_t limit(int16_t number, int16_t min, int16_t max);

struct {
  const char *label;
  const Icon icons[PROFILE_ICON_COUNT];
} const PROFILE_TYPES[PROFILE_TYPE_COUNT + 1] = {
  {
    .label="- Empty -",
    .icons={NO_ICON, NO_ICON, NO_ICON}
  },
  {
    .label="Moka Pot ",
    .icons={BIALETTI_ICON, ALESSI_ICON, VEV_VIGANO_ICON}
  },
  {
    .label="Fr Press ",
    .icons={BODUM_ICON, NO_ICON, NO_ICON}
  },
  {
    .label="Pour Over",
    .icons={CHEMEX_ICON, V60_ICON, NO_ICON}
  },
};

struct Profile {
  int8_t _type;
  int16_t _timer;
  int8_t _icon;

  bool isEmpty() {
    return _type == EMPTY;
  }

  const char* label() {
    return PROFILE_TYPES[_type].label;
  }

  uint16_t timer() {
    return isEmpty() ? 0 : _timer;
  }

  const Icon& icon() {
    return _icon == OFF ? NO_ICON : PROFILE_TYPES[_type].icons[_icon];
  }

  void changeType(int8_t delta) {
    int8_t next = limit(_type + delta, EMPTY, PROFILE_TYPE_COUNT);
    if (next != _type) {
      _type = next;
      _icon = 0;
      if (isEmpty()) {
        _timer = 0;
      } else if (_timer < MIN_TIMER) {
        _timer = MIN_TIMER;
      }
    }
  }

  void changeTimer(int16_t delta) {
    _timer = limit(_timer + delta, MIN_TIMER, MAX_TIMER);
  }

  void changeIcon(int8_t delta) {
    // Find max icon index for current profile type
    int8_t max = -1;
    while (max < PROFILE_ICON_COUNT - 1 && PROFILE_TYPES[_type].icons[max+1].data != NULL) { max++; }
    _icon = limit(_icon + delta, OFF, max);
  }
};

struct ProfilesClass {
  struct Data {
    int8_t profile;
    Profile profiles[MAX_PROFILE_COUNT];
  } data;

  struct Profile *current = NULL;

  void load() {
    EEPROM.get(0, data);
    current = getCurrentProfile();

    if (data.profile == UINT8_MAX) {
      this->reset();
    }
  }

  void save() {
    EEPROM.put(0, data);
  }

  void reset() {
    setProfile(NONE);
    for (uint8_t i = 0; i < MAX_PROFILE_COUNT; i++) {
      data.profiles[i] = {EMPTY, 0, OFF};
    }
    save();
  }

  Profile* getCurrentProfile() {
    return data.profile == NONE ? NULL : &(data.profiles[data.profile - 1]);
  }

  void setProfile(int8_t i) {
    data.profile = i;
    current = getCurrentProfile();
  }
  
  void changeProfile(int8_t delta) {
    setProfile(limit(data.profile + delta, NONE, MAX_PROFILE_COUNT));
  }
};

int16_t limit(int16_t number, int16_t min, int16_t max) {
  return number < min ? min : number > max ? max : number;
}

static ProfilesClass Profiles;

#endif