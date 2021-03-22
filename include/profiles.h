#ifndef __SETTINGS__
#define __SETTINGS__

#include <Arduino.h>
#include <EEPROM.h>

#include <icons.h>

#define OFF -1
#define MANUAL_MODE -1
#define MANUAL_TYPE -2

#define MAX_PROFILE_COUNT 9
#define PROFILE_TYPE_COUNT 3

#define MIN_TIMER 1000
#define MAX_TIMER 30000

int16_t limit(int16_t number, int16_t min, int16_t max);

struct Icon {
  const uint8_t* data;
  const uint8_t width;
  const uint8_t offset;
  const uint8_t page;
};

const Icon MANUAL_ICON = {manual_icon30x32, 30, 0, 0};
const Icon NO_ICON = {NULL};

struct {
  const char *label;
  const Icon icons[3];
} const PROFILE_TYPES[PROFILE_TYPE_COUNT] = {
  {
    .label="Moka Pot",
    .icons={	
      {bialetti_icon26x32, 26, 0, 0},
      {alessi_icon21x32, 21, 4, 0},
      {vev_vigano_icon21x32, 21, 2, 0},
    }
  },
  {
    .label="French Press",
    .icons={
      {bodum_icon21x32, 21, 2, 0},
      {NULL},
      {NULL},
    }
  },
  {
    .label="Pour Over",
    .icons={
      {chemex_icon23x32, 23, 1, 0},
      {v60_icon25x24, 25, 0, 1},
      {NULL},
    }
  },
};

struct Profile {
  int8_t _type;
  int16_t _timer;
  int8_t _icon;

  bool isManual() {
    return _type == MANUAL_TYPE;
  }

  bool isEmpty() {
    return _type == OFF;
  }

  const char* label() {
    if (isManual()) {
      return "Manual";
    } else if (isEmpty()) {
      return "- Empty -";
    } else {
      return PROFILE_TYPES[_type].label;
    }
  }

  uint16_t timer() {
    return isManual() || isEmpty() ? 0 : _timer;
  }

  const Icon& icon() {
    if (isManual()) {
      return MANUAL_ICON;
    } else if (isEmpty() || _icon == OFF) {
      return NO_ICON;
    } else {
      return PROFILE_TYPES[_type].icons[_icon];
    }
  }

  void changeType(int8_t delta) {
    _type = limit(_type + delta, OFF, PROFILE_TYPE_COUNT - 1);
    _icon = 0;
    if (isEmpty()) {
      _timer = 0;
    } else if (_timer < MIN_TIMER) {
      _timer = MIN_TIMER;
    }
  }

  void changeTimer(int16_t delta) {
    if (isManual() || isEmpty()) {
      return;
    }
    _timer = limit(_timer + delta, MIN_TIMER, MAX_TIMER);
  }

  void changeIcon(int8_t delta) {
    if (isManual() || isEmpty()) {
      return;
    }
    // Find max icon index for current profile
    int8_t max;
    for (max = -1; max < 2; max++) {
      if (PROFILE_TYPES[_type].icons[max+1].data == NULL) {
        break;
      }
    }
    _icon = limit(_icon + delta, OFF, max);
  }
};

Profile manual = {MANUAL_TYPE, 0, 0};

struct ProfilesClass {

  struct Data {
    int8_t current;
    Profile profiles[MAX_PROFILE_COUNT];
  } data;

  struct Profile *current = &manual;

  void load() {
    EEPROM.get(0, data);
    current = &getCurrentProfile();

    if (data.current == UINT8_MAX) {
      this->reset();
    }
  }

  void reset() {
    setProfile(MANUAL_MODE);
    for (uint8_t i = 0; i < MAX_PROFILE_COUNT; i++) {
      data.profiles[i] = {OFF, 0, OFF};
    }
    save();
  }
  
  void save() {
    EEPROM.put(0, data);
  }

  Profile& getCurrentProfile() {
    return data.current == MANUAL_MODE ? manual : data.profiles[data.current];
  }

  void setProfile(int8_t i) {
    data.current = i;
    current = &getCurrentProfile();
  }
  
  void changeProfile(int8_t delta) {
    setProfile(limit(data.current + delta, OFF, MAX_PROFILE_COUNT - 1));
  }
};

int16_t limit(int16_t number, int16_t min, int16_t max) {
  /* 
  * Return given number, limited to min and max.
  */
  if (number > max) {
    return max;
  } else if (number < min) {
    return min;
  } else {
    return number;
  }
}

static ProfilesClass Profiles;

#endif