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
        Logger(All, "Enqueued event sent from entity #%d.\n", event->sender()->id());
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
                Logger(All, "Deleted event sent from entity #%d.\n", event->sender()->id());
        }
}

Event::Event(EventType type, const Entity *sender, size_t nParams, double *params)
        : m_type(type), m_sender(sender), m_nParams(nParams), m_params(NULL), m_time(GetGlobalTime())
{
        if (m_nParams > 0) {
                m_params = new double[m_nParams];
                memcpy(m_params, params, m_nParams * sizeof(double));
        }
}

Event::Event(const Event& event)
        : m_type(event.m_type), m_sender(event.m_sender), m_nParams(event.m_nParams), m_params(NULL), m_time(event.m_time)
{
        if (m_nParams > 0) {
                m_params = new double[m_nParams];
                memcpy(m_params, event.m_params, m_nParams * sizeof(double));
        }
}

Event::~Event()
{
        if (m_nParams > 0)
                delete m_params;
}


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

size_t Event::nParams() const
{
        return m_nParams;
}

const double* Event::params() const
{
        return m_params;
}

double Event::param(uint pos) const
{
        if (pos < m_nParams)
                return m_params[pos];
        else
                throw "Index out of bounds.";
}

SpikeEvent::SpikeEvent(const Entity *sender, double weight)
        : Event(SPIKE, sender, 1, &weight)
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
	TerminateTrial();
}

} // namespace dynclamp

