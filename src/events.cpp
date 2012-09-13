/*=========================================================================
 *
 *   Program:     dynclamp
 *   Filename:    events.cpp
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

#include "events.h"
#include "entity.h"
#include "thread_safe_queue.h"
#include "engine.h"

namespace dynclamp {

/*! The queue where events are stored. */
ThreadSafeQueue<const Event*> eventsQueue;

void EnqueueEvent(const Event *event)
{
        eventsQueue.push_back(event);
        Logger(Debug, "Enqueued event sent from entity #%d.\n", event->sender()->id());
}

void ProcessEvents()
{
        uint i, j, nEvents, nPost;
        nEvents = eventsQueue.size();
        Logger(All, "There are %d events in the queue.\n", nEvents);
        for (i=0; i<nEvents; i++) {
                const Event *event = eventsQueue.pop_front();
                const std::vector<Entity*>& post = event->sender()->post();
                nPost = post.size();
                for (j=0; j<nPost; j++)
                        post[j]->handleEvent(event);
                delete event;
                Logger(Debug, "Deleted event sent from entity #%d.\n", event->sender()->id());
        }
}

Event::Event(EventType type, const Entity *sender)
        : m_type(type), m_sender(sender), m_time(GetGlobalTime())
{}

Event::Event(const Event& event)
        : m_type(event.m_type), m_sender(event.m_sender), m_time(event.m_time)
{}

EventType Event::type() const
{
        return m_type;
}

const Entity* Event::sender() const
{
        return m_sender;
}

double Event::time() const
{
        return m_time;
}

SpikeEvent::SpikeEvent(const Entity *sender)
        : Event(SPIKE, sender)
{}

TriggerEvent::TriggerEvent(const Entity *sender)
        : Event(TRIGGER, sender)
{}

ResetEvent::ResetEvent(const Entity *sender)
        : Event(RESET, sender)
{}

ToggleEvent::ToggleEvent(const Entity *sender)
        : Event(TOGGLE, sender)
{}

StopRunEvent::StopRunEvent(const Entity *sender)
        : Event(STOPRUN, sender)
{
	stopRun();
}

} // namespace dynclamp

