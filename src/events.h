/*=========================================================================
 *
 *   Program:     dynclamp
 *   Filename:    events.h
 *
 *   Copyright (C) 2012 Daniele Linaro
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *   
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *   
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *=========================================================================*/

#ifndef EVENTS_H
#define EVENTS_H

#include "utils.h"
#include <string>

/*!
 * \file events.h
 * \brief Contains the definition of the base class Event and of some derived classes.
 */

namespace dynclamp {

class Entity;

/*! This enumeration defines the various types of events that can be sent and received by entities. */
typedef enum {
        /*! A spike event is emitted by an object that inherits from dynclamp::neurons::Neuron
         * and is used to inform the receiver that the threshold for spiking was crossed. */
        SPIKE = 0,
        /*!
         * A trigger event can be emitted by any entity and it should be used to 
         * inform the receiver that an appropriate action should be started.
         * \sa Trigger, PeriodicTrigger
         */
        TRIGGER,
        /*!
         * A reset event can be emitted by any entity and it should be used to 
         * inform the receiver that its output should be reset.
         * \sa EventCounter for an example of how to handle a reset event.
         * \sa generators::Waveform for an example of a class that sends a reset event.
         */
        RESET,
        /*!
         * A toggle event can be emitted by any entity and it should be used to 
         * inform the receiver that it should toggle from one mode of operation to another.
         * \sa PID
         */
        TOGGLE,
        /*!
         * A stoprun event stops the experiment/simulation.
         */
        STOPRUN
} EventType;

#define NUMBER_OF_EVENT_TYPES 5
const std::string eventTypeNames[NUMBER_OF_EVENT_TYPES] = {"spike", "trigger", "reset", "toggle", "stoprun"};

/*!
 * \class Event
 * \brief A base class for representing events.
 *
 * Events are the way in which entities can communicate with each other. An event is described by
 * its type and the time at which it was created. Moreover, events contain a pointer to the entity
 * that sent them, which can be used by the receiving entity.
 *
 * All classes derived from Event should implement one of the event types described by
 * the enumeration EventType.
 */
class Event
{
public:
        /*!
         * Constructs an event of a given type, with a given sender. It automatically
         * initialises also the time at which the event was created.
         * \sa EventType
         */
        Event(EventType type, const Entity *sender);

        /*!
         * Constructs a copy of a given event. Also the creation time is the same,
         * despite the fact that the copy may be constructed successively.
         */
        Event(const Event& event);

        /*! Returns the type of the event. \sa EventType */
        EventType type() const;

        /*! Returns a pointer to the entity that emitted this event. */
        const Entity* sender() const;

        /*! Returns the time at which this event was emitted. */
        double time() const;

private:
        EventType m_type;
        const Entity *m_sender;
        double m_time;
};

/*!
 * \class SpikeEvent
 * \brief A class that implements an event sent when a neurons::Neuron crosses the spike
 * generation threshold.
 */
class SpikeEvent : public Event
{
public:
        /*! Constructs a SpikeEvent with a given sender. */
        SpikeEvent(const Entity *sender);
};

/*!
 * \class TriggerEvent
 * \brief A class that implements an event that should be used to communicate the receiver to start
 * some action.
 */
class TriggerEvent : public Event
{
public:
        /*! Constructs a TriggerEvent with a given sender. */
        TriggerEvent(const Entity *sender);
};

/*!
 * \class ResetEvent
 * \brief A class that implements an event that should be used to communicate the receiver to reset
 * its output.
 */
class ResetEvent : public Event
{
public:
        /*! Constructs a ResetEvent with a given sender. */
        ResetEvent(const Entity *sender);
};

/*!
 * \class ToggleEvent
 * \brief A class that implements an event that should be used to communicate the receiver to change
 * its output.
 */
class ToggleEvent : public Event
{
public:
        /*! Constructs a ToggleEvent with a given sender. */
        ToggleEvent(const Entity *sender);
};

/*!
 * \class StopRunEvent
 * \brief A class that implements an event that should be used to communicate that the experiment/simulation
 * should be halted.
 */
class StopRunEvent : public Event
{
public:
        /*! Constructs a StopRunEvent with a given sender. */
        StopRunEvent(const Entity *sender);
};

/*! Puts the event passed as an argument into the events queue. */
void EnqueueEvent(const Event *event);

/*!
 * This function is called at each time step of the experiment/simulation.
 * Its purpose is to go through the events queue and deliver the events to appropriate
 * receivers, according to the connections between entities. For example, if entity A is connected
 * to entities B and C, when A emits an event, this function will call the method
 * handleEvent of B and C, passing as a parameter a pointer to the event that was emitted by A.
 */
void ProcessEvents();

} // namespace dynclamp

#endif

