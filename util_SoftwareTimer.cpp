////////////////////////////////////////////////////////////////////////////////
/// @section LICENSE                                                         ///
///                                                                          ///
///        Distributed under the Boost Software License, Version 1.0.        ///
///             (See accompanying file LICENSE_1_0.txt or copy at            ///
///                  http://www.boost.org/LICENSE_1_0.txt)                   ///
///                                                                          ///
/// @file                                                                    ///
/// @author  Alejandro Morell Garcia                                         ///
/// @version 1.0                                                             ///
////////////////////////////////////////////////////////////////////////////////
#include "util/SoftwareTimer.h"
#include <util/time.h>
#include <Arduino.h>


namespace util {

SoftwareTimer::SoftwareTimer()
	: m_delayOffset(0)
//	, m_lastTick(millis())
	, m_waitingTick(false)
{
}

void SoftwareTimer::process() {
	if (m_waitingTick) {
		unsigned long current = millis();
//		if (elapsedTime(m_lastTick, current) >= m_delayOffset) {
		if (elapsedTime(getLastTick(), current) >= m_delayOffset) {
			m_waitingTick = false;
			doTick(current);
//			m_lastTick = current;
		}
	}
}

void SoftwareTimer::setNextTickTimer(const unsigned long &delay) {
	m_delayOffset = delay;
	m_waitingTick = true;
	Serial.print(F("nextTick="));
	Serial.println(delay);
}

} // namespace util
