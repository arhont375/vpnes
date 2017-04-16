/**
 * @file
 *
 * Defines basic device classess
 */
/*
 NES Emulator
 Copyright (C) 2012-2017  Ivanov Viktor

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along
 with this program; if not, write to the Free Software Foundation, Inc.,
 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

 */

#ifndef VPNES_INCLUDE_CORE_DEVICE_HPP_
#define VPNES_INCLUDE_CORE_DEVICE_HPP_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cassert>
#include <cstddef>
#include <utility>
#include <algorithm>
#include <list>
#include <unordered_map>
#include <set>
#include <string>
#include <vpnes/vpnes.hpp>

namespace vpnes {

namespace core {

/**
 * Base device class
 */
class CDevice {
protected:
	/**
	 * Default constructor
	 *
	 * Put as protected for converting into meta-class
	 */
	CDevice() = default;
public:
	/**
	 * Default destructor
	 */
	virtual ~CDevice() = default;
};

/**
 * Clock ticks type
 */
typedef std::size_t clock_t;

/**
 * Default clocked device
 */
class CClockedDevice: public CDevice {
protected:
	/**
	 * Current clock value
	 */
	clock_t m_Clock;

	/**
	 * Executes till reaching current clock value
	 */
	virtual void execute() = 0;
	/**
	 * Synchronizes the clock
	 *
	 * @param ticks New clock value
	 */
	virtual void sync(clock_t ticks) {
	}
public:
	/**
	 * Constructs the object
	 */
	CClockedDevice() :
			m_Clock() {
	}
	/**
	 * Destroys the object
	 */
	~CClockedDevice() = default;

	/**
	 * Advances the clock
	 *
	 * @param ticks End time
	 */
	virtual void simulate(clock_t ticks) {
		setClock(ticks);
		execute();
	}

	/**
	 * Gets current clock value
	 *
	 * @return Current clock value
	 */
	clock_t getClock() const {
		return m_Clock;
	}
	/**
	 * Sets the end point for clock
	 *
	 * @param ticks Amount of ticks
	 */
	void setClock(clock_t ticks) {
		if (ticks < m_Clock) {
			sync(ticks);
			m_Clock = ticks;
		}
	}
	/**
	 * Resets the clock by ticks amount
	 *
	 * @param ticks Amount of ticks
	 */
	virtual void resetClock(clock_t ticks) {
		m_Clock -= ticks;
	}
};

/**
 * Base device event class
 */
class CDeviceEvent {
protected:
	/**
	 * Time when firing
	 */
	clock_t m_Time;
	/**
	 * Event name
	 */
	const char *m_Name;
	/**
	 * Flag enabling the event
	 */
	bool m_Enabled;
	/**
	 * Default constructor
	 *
	 * Put as protected for converting into meta-class
	 *
	 * @param name Event name
	 * @param time Firing time
	 * @param enabled Enabled or not
	 */
	CDeviceEvent(const char *name, clock_t time, bool enabled) :
			m_Time(time), m_Name(name), m_Enabled(enabled) {
	}
public:
	/**
	 * Default destructor
	 */
	virtual ~CDeviceEvent() = default;
	/**
	 * Synchronizes the clock
	 *
	 * @param ticks Amount of ticks
	 */
	void sync(clock_t ticks) {
		m_Time -= ticks;
	}
	/**
	 * Fires the trigger
	 */
	virtual void fire() = 0;
	/**
	 * Gets time when event will fire
	 *
	 * @return Time when firing
	 */
	clock_t getFireTime() const {
		return m_Time;
	}
	/**
	 * Checks if event is enabled
	 *
	 * @return True if enabled
	 */
	bool isEnabled() const {
		return m_Enabled;
	}
};

/**
 * Event-based device
 */
class CEventDevice: public CClockedDevice {
public:
	/**
	 * Local event
	 */
	class CEvent: public CDeviceEvent {
	protected:
		/**
		 * Pointer to associated device
		 */
		CEventDevice *m_Device;
	public:
		/**
		 * Constructs the object
		 *
		 * @param name Name of event
		 * @param time Event fire time
		 * @param enabled Enabled or not
		 * @param device Associated device
		 */
		CEvent(const char *name, clock_t time, bool enabled,
				CEventDevice *device) :
				CDeviceEvent(name, time, enabled), m_Device(device) {
		}
		/**
		 * Destroys the object
		 */
		~CEvent() = default;
		/**
		 * Updates fire time
		 *
		 * @param time Time when fired
		 */
		void setFireTime(clock_t time) {
			m_Time = time;
			m_Device->updateBack(this);
		}
		/**
		 * Enables or disables the event
		 *
		 * @param enabled True to enable, false to disable
		 */
		void setEnabled(bool enabled) {
			m_Enabled = enabled;
			m_Device->updateBack(this);
		}
	};
	/**
	 * Template for local events
	 */
	template<class T>
	class CLocalEvent: public CEvent {
	public:
		/**
		 * Local event handler
		 *
		 * @param Occurred event
		 */
		typedef void (T::*local_trigger_t)(CEvent *);
	private:
		/**
		 * Saved local event handler
		 */
		local_trigger_t m_LocalTrigger;
	public:
		/**
		 * Constructs the object
		 *
		 * @param name Name of event
		 * @param time Event fire time
		 * @param enabled Enabled or not
		 * @param device Associated device
		 * @param trigger Trigger that will be fired
		 */
		CLocalEvent(const char *name, clock_t time, bool enabled, T *device,
				local_trigger_t trigger) :
				CEvent(name, time, enabled, device), m_LocalTrigger(trigger) {
		}
		/**
		 * Destroys the object
		 */
		~CLocalEvent() = default;
		/**
		 * Trigger for local event handler
		 */
		void fire() {
			(static_cast<T *>(m_Device)->*m_LocalTrigger)(this);
		}
	};
private:
	/**
	 * Saved event data
	 */
	struct SEventData {
		/**
		 * Link to the event
		 */
		CEvent *m_Event;
		/**
		 * Saved time
		 */
		std::size_t m_Time;
		/**
		 * Saved enabling flag
		 */
		bool m_Enabled;
		/**
		 * Constructs the object
		 *
		 * @param event Event to add
		 */
		SEventData(CEvent *event) {
			m_Event = event;
			update();
		}

		/**
		 * Updates the local data
		 */
		void update() {
			m_Time = m_Event->getFireTime();
			m_Enabled = m_Event->isEnabled();
		}
		/**
		 * Special routine for comparison of event data pointers
		 *
		 * @param left Left operand
		 * @param right Right operand
		 * @return *Left < *Right
		 */
		static bool compare(SEventData * const left, SEventData * const right) {
			assert(right->m_Enabled && left->m_Enabled);
			return left->m_Time < right->m_Time;
		}
	};
	/**
	 * Event data mapped to event
	 */
	typedef std::unordered_map<CEvent *, SEventData> EventMap;
	/**
	 * Event queue
	 */
	typedef std::set<SEventData *,
			bool (*)(SEventData * const, SEventData * const)> EventQueue;
	/**
	 * Event data mapped to event
	 */
	EventMap m_EventData;
	/**
	 * Event queue
	 */
	EventQueue m_EventQueue;
protected:
	/**
	 * Local end time
	 */
	clock_t m_LocalTime;

	/**
	 * Updates clock value
	 */
	void updateClock() {
		clock_t newClock = m_Clock;
		EventQueue::const_reverse_iterator iter = m_EventQueue.crbegin();

		if (iter == m_EventQueue.crend()) {
			newClock = m_LocalTime;
		} else {
			newClock = (*iter)->m_Time;
			if (newClock > m_LocalTime) {
				newClock = m_LocalTime;
			}
		}
		setClock(newClock);
	}
	/**
	 * Generates ticks
	 *
	 * @return New clock value
	 */
	clock_t generateTicks() {
		EventQueue::const_reverse_iterator iter = m_EventQueue.crbegin();
		assert(iter != m_EventQueue.crend());
		return (*iter)->m_Time;
	}
	/**
	 * Fires events that are available to
	 */
	void fireEvents() {
		for (;;) {
			EventQueue::const_reverse_iterator iter = m_EventQueue.crbegin();
			if (iter == m_EventQueue.crend() || (*iter)->m_Time > m_Clock) {
				break;
			}
			(*iter)->m_Event->fire();
		}
	}
public:
	/**
	 * Constructs the object
	 */
	CEventDevice() :
			m_EventData(), m_EventQueue(SEventData::compare), m_LocalTime() {
	}
	/**
	 * Destroys the object
	 */
	~CEventDevice() = default;

	/**
	 * Advances the clock
	 *
	 * @param ticks End time
	 */
	void simulate(clock_t ticks) {
		m_LocalTime = ticks;
		do {
			updateClock();
			execute();
			fireEvents();
		} while (m_Clock < m_LocalTime);
	}
	/**
	 * Update trigger for event
	 *
	 * @param event Event
	 */
	void updateBack(CEvent * event) {
		EventMap::iterator iter = m_EventData.find(event);
		assert(iter != m_EventData.end());
		SEventData &eventData = iter->second;
		if (eventData.m_Enabled) {
			m_EventQueue.erase(&eventData);
		}
		eventData.update();
		if (eventData.m_Enabled) {
			m_EventQueue.insert(&eventData);
		}
		updateClock();
	}
	/**
	 * Resets the clock
	 *
	 * @param ticks Amount of ticks
	 */
	void resetClock(clock_t ticks) {
		CClockedDevice::resetClock(ticks);
		for (auto &event : m_EventData) {
			SEventData &eventData = event.second;
			eventData.m_Event->sync(ticks);
			eventData.m_Time = eventData.m_Event->getFireTime();
		}
	}
	/**
	 * Registers new event
	 *
	 * @param event New event
	 */
	void registerEvent(CEvent *event) {
		assert(m_EventData.find(event) == m_EventData.end());
		auto newData = m_EventData.emplace(event, event);
		SEventData &eventData = newData.first->second;
		if (eventData.m_Enabled) {
			m_EventQueue.insert(&eventData);
			updateClock();
		}
	}
};

/**
 * Device that can run with self-generated ticks
 */
class CGeneratorDevice: public CEventDevice {
private:
	/**
	 * Enabling flag
	 */
	bool m_Enabled;
public:
	/**
	 * Constructs the object
	 *
	 * @param enabled Enable or not
	 */
	CGeneratorDevice(bool enabled) :
			m_Enabled(enabled) {
	}
	/**
	 * Destroys the object
	 */
	~CGeneratorDevice() = default;

	/**
	 * Run while enabled
	 */
	void simulate() {
		while (m_Enabled) {
			setClock(generateTicks());
			execute();
		}
	}
	/**
	 * Checks if the device is enabled or not
	 *
	 * @return True if enabled
	 */
	bool isEnabled() const {
		return m_Enabled;
	}
	/**
	 * Enables or disables the device
	 *
	 * @param enabled True to enable, false to disable
	 */
	void setEnabled(bool enabled) {
		m_Enabled = enabled;
	}

};

/**
 * Basic event manager
 */
class CEventManager {
private:
	/**
	 * Map of events and their names
	 */
	typedef std::unordered_map<std::string, CDeviceEvent *> EventMap;
	/**
	 * Pairs of devices and their events
	 */
	typedef std::pair<CEventDevice *, CDeviceEvent *> EventPair;
	/**
	 * Array of pairs device-event
	 */
	typedef std::list<EventPair> EventArr;
	/**
	 * Map of events and their names
	 */
	EventMap eventMap;
	/**
	 * Array of pairs device-event
	 */
	EventArr events;
public:
	/**
	 * Constructs the object
	 */
	CEventManager() :
			eventMap(), events() {
	}
	/**
	 * Destroys the object
	 */
	~CEventManager() {
		for (auto &v : events) {
			delete v.second;
		}
	}

	/**
	 * Constructs an event and registers it
	 *
	 * @param device Event's owner
	 * @param name Event name
	 * @param time Event time
	 * @param enabled Enabled or not
	 * @param args Other arguments to pass to constructor
	 * @return Constructed event
	 */
	template<class T, typename ... TArgs>
	typename T::CEvent registerEvent(T *device, const char *name, clock_t time,
			bool enabled, TArgs ... args) {
		assert(eventMap.find(name) == eventMap.end());
		typename T::CEvent *event = new typename T::template CLocalEvent<T>(
				name, time, enabled, device, args...);
		eventMap.emplace(name, event);
		events.emplace(device, event);
		device->registerEvent(event);
		return event;
	}
	/**
	 * Looks up for an event
	 *
	 * @param name Name of event
	 * @return Found event
	 */
	CDeviceEvent *getEvent(const char *name) {
		EventMap::const_iterator iter = eventMap.find(name);
		assert(iter != eventMap.end());
		return iter->second;
	}
	/**
	 * Unregisters all device's events
	 *
	 * @param device Events' owner
	 */
	void unregisterEvents(CEventDevice *device) {
		for (EventArr::iterator iter = std::find_if(events.begin(),
				events.end(), [&](EventPair &v) -> bool {
					return v.first == device;
				}); iter != events.end(); iter = events.erase(iter)) {
			delete iter->second;
		}
	}
};

}

}

#endif /* VPNES_INCLUDE_CORE_DEVICE_HPP_ */
