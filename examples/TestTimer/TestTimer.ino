////////////////////////////////////////////////////////////////////////////////
/// @section LICENSE                                                         ///
///                                                                          ///
///        Distributed under the Boost Software License, Version 1.0.        ///
///             (See accompanying file LICENSE_1_0.txt or copy at            ///
///                  http://www.boost.org/LICENSE_1_0.txt)                   ///
///                                                                          ///
/// @file                                                                    ///
/// @author  Alejandro Morell Garcia <alejandro.morell@gmail.com>            ///
/// @version 1.0                                                             ///
////////////////////////////////////////////////////////////////////////////////

#include <TimerLib.h>
#include <UtilLib.h>
#include <SRUtilLib.h>

#include <util/time.h>
#include <util/Array.h>

#include <util/SoftwareTimer.h>
using util::SoftwareTimer;
SoftwareTimer timer;


void oneTimeCallback();
void showMillis(const __FlashStringHelper *);
void repeatCallback(void *);


using util::TimerTicket;
extern HardwareSerial Serial;
TimerTicket myTicket, myTicket2, myTicket3;
uint16_t counter;

class Counter {
public:
	Counter() : m_counter(0) {}
	void incCount() {
		showMillis(F("Counter::incCount"));
		Serial.print(F(", counter="));
		Serial.println(m_counter);
		m_counter++;
//		delay(1000);
	}

private:
	uint16_t m_counter;
};

Counter counterObj;

void setup() {
	Serial.begin(9600);

	Serial.println(F("setup timer"));
	timer.setup();

	// Set a one time callback
	myTicket.setFunctionCallback<&oneTimeCallback>();
	timer.schedOneTime(myTicket, 3, TimerTicket::SECONDS);

	// Repeat repeatcallback each 1 second
	counter = 0;
	myTicket2.setFunctionDataCallback<&repeatCallback>(&counter);
	timer.schedRepeat(myTicket2, 3, TimerTicket::SECONDS, 1, TimerTicket::SECONDS);

	// Repeat Counter.incCount each minute
	myTicket3.setMethodCallback<Counter, &Counter::incCount>(&counterObj);
	timer.schedRepeat(myTicket3, 1, TimerTicket::MINUTES);

	timer.showTicketList(Serial);

	Serial.println(F("start timer"));
	delay(1000);
	timer.start();
}

void loop() {
	timer.process();
}

void showMillis(const __FlashStringHelper *name) {
	unsigned long m = millis();
	Serial.print('[');
	Serial.print(name);
	Serial.print(F("]["));
	Serial.print(m);
	Serial.print(F("ms]"));
}

void oneTimeCallback() {
	showMillis(F("oneTimeCallback"));
	Serial.println();
}

void repeatCallback(void *data) {
	showMillis(F("repeatCallback"));
	uint16_t *counter = reinterpret_cast<uint16_t *>(data);
	Serial.print(F(", counter="));
	Serial.println(*counter);
	(*counter)++;
}
