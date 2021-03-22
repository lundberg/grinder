#ifndef __BUTTON__
#define __BUTTON__

#include <Arduino.h>

#define BUTTON_DIGITAL_UP HIGH
#define BUTTON_DIGITAL_DOWN LOW

//template<class T> inline T operator~ (T a) { return (T)~(int)a; }
//template<class T> inline T operator^ (T a, T b) { return (T)((int)a ^ (int)b); }
//template<class T> inline T& operator|= (T& a, T b) { return (T&)((int&)a |= (int)b); }
//template<class T> inline T& operator&= (T& a, T b) { return (T&)((int&)a &= (int)b); }
//template<class T> inline T& operator^= (T& a, T b) { return (T&)((int&)a ^= (int)b); }
template<class T> inline T operator| (T a, T b) { return (T)((int)a | (int)b); }
template<class T> inline T operator& (T a, T b) { return (T)((int)a & (int)b); }

class Button {

public:
	enum class Event {
		NONE         = 0,
		UP           = 1 << 0,
		DOWN         = 1 << 1,
		CLICK        = 1 << 2,
		LONG_HOLD    = 1 << 3,
		ALL = UP | DOWN | CLICK | LONG_HOLD
	};

	Button(uint8_t pin, Event events = Event::CLICK, int16_t analogThreshold = 0);

	void begin();
	void watch(Event events);
	bool watches(Event event);
	void setHoldDuration(unsigned long ms);

	Event peek();
	Event read();
	bool tick();

private:
	uint8_t pin;
	int16_t analogThreshold;
	uint8_t state = BUTTON_DIGITAL_UP;

	Event events;
	Event event = Event::NONE;
	Event lastEvent = Event::NONE;

	uint16_t holdDuration = 1500;
	uint32_t startTime = 0;

	uint8_t readState();
};

#endif