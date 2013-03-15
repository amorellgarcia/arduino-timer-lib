////////////////////////////////////////////////////////////////////////////////
/// @section LICENSE                                                         ///
///                                                                          ///
///        Distributed under the Boost Software License, Version 1.0.        ///
///             (See accompanying file LICENSE_1_0.txt or copy at            ///
///                  http://www.boost.org/LICENSE_1_0.txt)                   ///
///                                                                          ///
/// @file                                                                    ///
/// @author  Alejandro Morell Garcia (http://github.com/amorellgarcia)       ///
/// @version 1.0                                                             ///
////////////////////////////////////////////////////////////////////////////////

#ifndef UTIL_SOFTWARETIMER_H_
#define UTIL_SOFTWARETIMER_H_

#include "Timer.h"

namespace util {

/**
 * Timer software implementation.
 * Application main loop must call @a process method to check pending tickets.
 * Precision of this timer is limited by the time elapsed between each call to
 * @a process method.
 */
class SoftwareTimer : public Timer {
public:
	/**
	 * Default constructor.
	 */
	SoftwareTimer();

	/**
	 * Checks pending tickets and executes them.
	 * Precision of timer is directly related to the delay between each call of
	 * this method.
	 * Usually this method is called in each iteration of application's main
	 * loop.
	 */
	void process();

private:
	void lowLevelSetup() {}
	void lock() {}
	void unlock() {}
	void setNextTickTimer(const unsigned long &);

private:
	unsigned long m_delayOffset;
//	unsigned long m_lastTick;
	bool m_waitingTick;
};

} // namespace util

#endif // UTIL_SOFTWARETIMER_H_
