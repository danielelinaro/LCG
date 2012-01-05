#ifndef EVENTS_H
#define EVENTS_H

#include "utils.h"

#define SPIKE_DELAY 2e-3

namespace dynclamp {

class Entity;

typedef enum _event_type {
        SPIKE = 0
} EventType;

class Event
{
public:
        Event(EventType type, const Entity *sender, double timeout);

        EventType type() const;
        double timeout() const;
        const Entity* sender() const;
        bool hasExpired() const;

        void decreaseTimeout(double dt);

private:
        EventType m_type;
        double m_timeout;
        const Entity *m_sender;
};

class SpikeEvent : public Event
{
public:
        SpikeEvent(const Entity *sender, double timeout = SPIKE_DELAY);
};

void EnqueueEvent(const Event *event);
void ProcessEvents(double dt = GetGlobalDt());

} // namespace dynclamp

#endif

