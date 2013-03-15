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
#include "util/Timer.h"
#include "util/time.h"
#include "util/detail/pstrings.h"
#include "util/bitfield.h"
#include "util/pgm_space.h"
#include <Arduino.h>
#include <stddef.h>

#define assert(x)
#define SECONDS_TO_MILLIS(x) 	(x) * 1000UL
#define MINUTES_TO_MILLIS(x) 	(x) * 1000UL * 60UL
#define HOURS_TO_MILLIS(x) 		(x) * 1000UL * 60UL * 60UL
#define DAYS_TO_MILLIS(x) 		(x) * 1000UL * 60UL * 60UL * 24UL

namespace util {

namespace timer_detail {
	PGMSPACE_STRING(MILLIS,  "ms");
	PGMSPACE_STRING(SECONDS, "s");
	PGMSPACE_STRING(MINUTES, " mins");
	PGMSPACE_STRING(HOURS,   "h");
	PGMSPACE_STRING(DAYS,    " days");
	PGMSPACE_STRING(UNKNOWN, "");
	PGMSPACE_ARRAY(PgmSpaceString, UNIT_STRING_LIST,
			MILLIS, SECONDS, MINUTES, HOURS, DAYS);
}

static const __FlashStringHelper *getUnitsString(TimerTicket::units_t unit) {
	switch (unit) {
	case TimerTicket::MILLIS ... TimerTicket::DAYS:
		return timer_detail::UNIT_STRING_LIST[unit];
	default:
		return timer_detail::UNKNOWN;
	}
}

bool TimerTicket::isScheduled() const {
	return isFlagEnabled(FLAG_TICKET_SCHEDULED);
}

void TimerTicket::printTo(Print &p) const {
	p.print(F("{delayOffset="));
	p.print(m_delayOffset, 10);
	p.print(F(", period="));
	p.print(m_period, 10);
	p.print(getUnitsString(getPeriodUnits()));
	p.print(F(", flags=0x"));
	p.print(m_flags, 16);
	p.print(F(", next_ticket=0x"));
	p.print((uintptr_t)m_next_ticket, 16);
	p.print('}');
}

inline bool TimerTicket::isFlagEnabled(flags_t flag) const {
	return ((this->m_flags & flag) != 0);
}

inline void TimerTicket::setFlag(flags_t flag) {
	m_flags = static_cast<flags_t>(m_flags | flag);
}
inline void TimerTicket::clearFlag(flags_t flag) {
	m_flags = static_cast<flags_t>(m_flags & ~flag);
}

TimerTicket::units_t TimerTicket::getPeriodUnits() const {
	return static_cast<TimerTicket::units_t>((m_flags & MASK_UNITS) >> OFFSET_UNITS);
}

void TimerTicket::setScheduled(bool value) {
	if (value) {
		setFlag(FLAG_TICKET_SCHEDULED);
	} else {
		clearFlag(FLAG_TICKET_SCHEDULED);
	}
}

void TimerTicket::setDelayOffset(uint16_t delay, units_t units) {
	switch (units) {
	case MILLIS:
		m_delayOffset = delay;
		break;
	case SECONDS:
		m_delayOffset = SECONDS_TO_MILLIS(delay);
		break;
	case MINUTES:
		m_delayOffset = MINUTES_TO_MILLIS(delay);
		break;
	case HOURS:
		m_delayOffset = HOURS_TO_MILLIS(delay);
		break;
	case DAYS:
		m_delayOffset = DAYS_TO_MILLIS(delay);
		break;
	}
}

void TimerTicket::setPeriodUnits(units_t units) {
	clearFlag(MASK_UNITS);
	setFlag(static_cast<flags_t>(units << OFFSET_UNITS));
}

Timer::Timer()
	: m_lastTick(0)
	, m_firstTicket(NULL)
	, m_running(false)
{
}

void Timer::showTicketList(Print &p) const {
	p.print(F("list={"));
	TimerTicket *ticket = m_firstTicket;
	if (ticket != NULL) {
		ticket->printTo(p);
		for (ticket = ticket->m_next_ticket; ticket != NULL; ticket = ticket->m_next_ticket) {
			p.print(detail::COMMA_SEP);
			ticket->printTo(p);
		}
	}
	Serial.println('}');
}


bool Timer::schedOneTime(TimerTicket &ticket, time_t delay, TimerTicket::units_t units) {
	return schedRepeat(ticket, delay, units, 0, TimerTicket::MILLIS);
}

bool Timer::schedRepeat(TimerTicket &ticket, time_t delayOffset, TimerTicket::units_t delayUnits, time_t period, TimerTicket::units_t periodUnits) {
	lock();
	ticket.setDelayOffset(delayOffset, delayUnits);
	ticket.m_period = period;
	ticket.setPeriodUnits(periodUnits);
	ticket.m_delayOffset += elapsedTime(m_lastTick, millis());

	addTicket(ticket);

	unlock();
	return true;
}

bool Timer::schedRepeat(TimerTicket &ticket, time_t period, TimerTicket::units_t periodUnits) {
	return schedRepeat(ticket, 0, TimerTicket::MILLIS, period, periodUnits);
}

void Timer::doTick(const unsigned long &currentMs) {
	lock();
	updateSchedule(currentMs);
	while (m_firstTicket != NULL && m_firstTicket->m_delayOffset == 0) {
		TimerTicket *ticket = m_firstTicket;
		m_firstTicket = m_firstTicket->m_next_ticket;

		if (ticket->m_delegate) {
			ticket->m_delegate();
		}

		ticket->m_next_ticket = NULL;
		ticket->setScheduled(false);
		if (ticket->m_period != 0) {
			ticket->setDelayOffset(ticket->m_period, ticket->getPeriodUnits());
			addTicket(*ticket);
		}
	}

	if (m_firstTicket != NULL && m_running) {
		setNextTickTimer(m_firstTicket->m_delayOffset/* - elapsedTime(currentMs, millis())*/);
	}
	unlock();
}


void Timer::setup() {
	lowLevelSetup();
}
void Timer::start() {
	lock();
	if (!m_running) {
		m_running = true;
		if (m_firstTicket != NULL) {
			setNextTickTimer(m_firstTicket->m_delayOffset);
		}
	}
	unlock();
}

void Timer::stop() {
	lock();
	if (m_running) {
		m_running = false;
	}
	unlock();
}

TimerTicket *Timer::findPreviousTicket(TimerTicket &ticket) {
	for (TimerTicket *current = m_firstTicket, *previous = NULL;
			current != NULL;
			previous = current, current = current->m_next_ticket)
	{
		if (current == &ticket) {
			return previous;
		}
	}

	return NULL;
}


void Timer::removeNextTicket(TimerTicket &ticket) {
	TimerTicket *next = ticket.m_next_ticket;
	if (next != NULL) {
		ticket.m_next_ticket = next->m_next_ticket;
		next->m_next_ticket = NULL;
		next->clearFlag(TimerTicket::FLAG_TICKET_SCHEDULED);
	}
}

void Timer::removeTicket(TimerTicket &ticket) {
	if (&ticket == m_firstTicket) {
		m_firstTicket = m_firstTicket->m_next_ticket;
	} else {
		TimerTicket *previous = findPreviousTicket(ticket);
		if (previous != NULL) {
			removeNextTicket(*previous);
		}
	}
	ticket.setScheduled(false);
}

void Timer::addTicket(TimerTicket &ticket) {
	if (ticket.isScheduled()) {
		removeTicket(ticket);
	}

	ticket.setScheduled(true);
	if (m_firstTicket == NULL) {
		m_firstTicket = &ticket;
		ticket.m_next_ticket = NULL;
	} else {
		if (ticket.m_delayOffset <= m_firstTicket->m_delayOffset) {
			m_firstTicket->m_delayOffset -= ticket.m_delayOffset;
			ticket.m_next_ticket = m_firstTicket;
			m_firstTicket = &ticket;
		} else {
			TimerTicket *current = m_firstTicket;
			for (	TimerTicket *next = current->m_next_ticket;
					next != NULL;
					)
			{
				ticket.m_delayOffset -= current->m_delayOffset;

				if (ticket.m_delayOffset < next->m_delayOffset) {
					next->m_delayOffset -= ticket.m_delayOffset;
					ticket.m_next_ticket = next;
					current->m_next_ticket = &ticket;
					return;
				}

				current = next;
				next = current->m_next_ticket;
			}

			ticket.m_next_ticket = NULL;
			current->m_next_ticket = &ticket;
		}
	}
}

void Timer::updateSchedule(const unsigned long &newTick) {
	unsigned long elapsed = elapsedTime(m_lastTick, newTick);

	for (TimerTicket *ticket = m_firstTicket; ticket != NULL; ticket = ticket->m_next_ticket) {
		if (ticket->m_delayOffset >= elapsed) {
			ticket->m_delayOffset -= elapsed;
			break;
		} else {
			ticket->m_delayOffset = 0;
			elapsed -= ticket->m_delayOffset;
		}
	}

	m_lastTick = newTick;
}

} // namespace util
