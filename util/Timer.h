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
#ifndef UTIL_TIMER_H_
#define UTIL_TIMER_H_

#include <util/Print.hpp>
#include <stdint.h>
#include <srutil/delegate.hpp>

namespace util {


/**
 * Used by @a Timer class to store a scheduled call-back execution.
 * A ticket can be scheduled only by a @a Timer at each time.
 *
 * Be careful when you change call-back for an scheduled ticket since it can
 * have a undefined behavior.
 */
class TimerTicket {
public:
	/**
	 *
	 */
	enum units_t {
		MILLIS, //!< MILLIS time is milliseconds
		SECONDS,//!< SECONDS time is in seconds
		MINUTES,//!< MINUTES time is in minutes
		HOURS,  //!< HOURS time is in hours
		DAYS,   //!< DAYS time is in days. Arduino only supports until 51 days
	};

public:
	/**
	 * Check if this ticket is scheduled for execution in a timer.
	 *
	 * @return true if scheduled, false otherwise.
	 */
	bool isScheduled() const;

	/**
	 * Prints ticket info to @a Print object
	 *
	 * @param p @a Print object where to print. Usually is @a Serial.
	 */
	void printTo(Print &p) const;

	/**
	 * Set a function as call-back.
	 * Function must have the following declaration:
	 * @code void func() @endcode
	 *
	 * Usage:
	 * @code
	 * void myCallback();
	 * // ...
	 * timer.setFunctionCallback<&myCallback>();
	 * @endcode
	 *
	 * Warning: changing call-back for an scheduled ticket is allowed but
	 * caution must be taken because @a Timer can execute it before this method
	 * returns.
	 */
	template <void func()>
	void setFunctionCallback();

	/**
	 * Set a function that receives data as call-back.
	 * Function must have the following declaration:
	 * @code void func(void *data) @endcode
	 *
	 * Usage:
	 * @code
	 * void myDataCallback(void *data);
	 * // ...
	 * void *myData = &data;
	 * timer.setFunctionDataCallback<&myCallback>(myData);
	 * @endcode
	 *
	 * Warning: changing call-back for an scheduled ticket is allowed but
	 * caution must be taken because @a Timer can execute it before this method
	 * returns.
	 */
	template <void func(void *)>
	void setFunctionDataCallback(void *data);

	/**
	 * Set a member method as callback.
	 * Method must have the following declaration:
	 * @code void myClass::func() @endcode
	 *
	 * Usage:
	 * @code
	 * class MyClass {
	 * // ...
	 *     void myMethod();
	 * // ...
	 * };
	 *
	 * MyClass obj;
	 * timer.setMethodCallback<MyClass, &MyClass::myMethod>(&obj);
	 * @endcode
	 *
	 * Warning: changing call-back for an scheduled ticket is allowed but
	 * caution must be taken because @a Timer can execute it before this method
	 * returns.
	 */
	template <typename T, void (T::*TMethod)()>
	void setMethodCallback(T *object);

private:
	typedef srutil::delegate<void ()> delegate_t;
	enum flags_t {
		OFFSET_UNITS = 0,
		MASK_UNITS = 0x3 << OFFSET_UNITS,
		OFFSET_FIRST_FLAG = 2,
		FLAG_TICKET_SCHEDULED = 1 << OFFSET_FIRST_FLAG,
	};

	bool isFlagEnabled(flags_t flag) const;
	void setFlag(flags_t flag);
	void clearFlag(flags_t flag);
	units_t getPeriodUnits() const;

	void setScheduled(bool value);
	void setDelayOffset(uint16_t delay, units_t units);
	void setPeriodUnits(units_t units);

	friend class Timer;
private:
	unsigned long m_delayOffset;
	TimerTicket *m_next_ticket;
	delegate_t m_delegate;
	uint16_t m_period;
	flags_t m_flags;
};

template <void func()>
inline void TimerTicket::setFunctionCallback() {
	m_delegate = delegate_t::from_function<func>();
}

template <void func(void *)>
inline void TimerTicket::setFunctionDataCallback(void *data) {
	m_delegate = delegate_t::from_function_data<func>(data);
}

template <typename T, void (T::*TMethod)()>
inline void TimerTicket::setMethodCallback(T *object) {
	m_delegate = delegate_t::from_method<T, TMethod>(object);
}


/**
 * Abstract Timer class used by platform-specific implementations.
 */
class Timer {
public:
	typedef uint16_t time_t;

public:
	/**
	 * Default constructor.
	 */
	Timer();

	/**
	 * Schedule a ticket for single execution.
	 * Ticket will be executed after a @a delay time. If timer is running,
	 * elapsed time counts since this method was called. If not running, elapsed
	 * time counts since timer is started.
	 *
	 * If @a ticket is already scheduled, this method does nothing and returns
	 * false.
	 *
	 * @param ticket ticket to use in execution.
	 * @param delay delay time
	 * @param units units of @a delay.
	 * @return true if scheduled, false otherwise.
	 *
	 * @see TimerTicket::units_t
	 */
	bool schedOneTime(TimerTicket &ticket, time_t delay, TimerTicket::units_t units = TimerTicket::MILLIS);

	/**
	 * Schedule a ticket for repeated execution.
	 * For first execution, this schedule works as @a schedOneTime. After that
	 * @a period is used to calculate delays between each execution.
	 *
	 * If @a ticket is already scheduled, this method does nothing and returns
	 * false.
	 *
	 * @param ticket ticket to use in execution.
	 * @param delay delay time
	 * @param delayUnits units of @a delay.
	 * @param period delay time between executions
	 * @param periodUnits units of @a period.
	 * @return true if scheduled, false otherwise.
	 *
	 * @see schedOneTime
	 * @see TimerTicket::units_t
	 */
	bool schedRepeat(TimerTicket &ticket, time_t delay, TimerTicket::units_t delayUnits, time_t period, TimerTicket::units_t periodUnits);

	/**
	 * Schedule a ticket for repeated execution.
	 * For first execution, this schedule works as @a schedOneTime with a delay
	 * of 0ms. After that @a period is used to calculate delays between each
	 * execution.
	 *
	 * If @a ticket is already scheduled, this method does nothing and returns
	 * false.
	 *
	 * @param ticket ticket to use in execution.
	 * @param period delay time between executions
	 * @param periodUnits units of @a period.
	 * @return true if scheduled, false otherwise.
	 *
	 * @see schedOneTime
	 * @see TimerTicket::units_t
	 */
	bool schedRepeat(TimerTicket &ticket, time_t period, TimerTicket::units_t periodUnits);

	/**
	 * Setups timer.
	 */
	void setup();

	/**
	 * Starts timer.
	 */
	void start();

	/**
	 * Stops timer.
	 */
	void stop();

	/**
	 * Check is timer is running/started.
	 *
	 * @return true if running/started, false otherwise.
	 */
	bool isRunning() const;

	/**
	 * Prints a list with all scheduled tickets.
	 * This method purpose is only debugging.
	 *
	 * @param p @a Print object where to print list.
	 */
	void showTicketList(Print &p) const;

protected:
	void doTick(const unsigned long &currentMs);

	/**
	 * Setup low-level functionality used for calling @a doTick.
	 */
	virtual void lowLevelSetup() = 0;

	/**
	 * Lock low-level functionality used for calling @a doTick.
	 * The reason for this method is to avoid race-conditions when timer is
	 * controlled with various execution threads or hardware interrupts.
	 *
	 * @see unlock()
	 */
	virtual void lock() = 0;

	/**
	 * Unlock low-level functionality used for calling @a doTick.
	 * The reason for this method is to avoid race-conditions when timer is
	 * controlled with various execution threads or hardware interrupts.
	 *
	 * @see lock()
	 */
	virtual void unlock() = 0;

	/**
	 * Method called from timer when next tick delay is calculated.
	 * Low-level implementation must call @a doTick when @a tickDelay
	 * milliseconds has been elapsed since the calling of this method.
	 *
	 * @param tickDelay time (in milliseconds) to wait before executing
	 * 	@a doTick.
	 */
	virtual void setNextTickTimer(const unsigned long &tickDelay) = 0;

	const unsigned long &getLastTick() const;

private:
	TimerTicket *findPreviousTicket(TimerTicket &ticket);
	void removeNextTicket(TimerTicket &ticket);
	void removeTicket(TimerTicket &ticket);
	void addTicket(TimerTicket &ticket);
	void updateSchedule(const unsigned long &currenMs);

private:
	unsigned long m_lastTick;
	TimerTicket *m_firstTicket;
	bool m_running;
};

inline bool Timer::isRunning() const {
	return m_running;
}

inline const unsigned long &Timer::getLastTick() const {
	return m_lastTick;
}


} // namespace util


template <>
inline void PrintValue<util::TimerTicket>(Print &p, const util::TimerTicket &ticket) {
	ticket.printTo(p);
}


#endif // UTIL_TIMER_H_
