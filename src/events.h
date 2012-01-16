#ifndef EVENTS_H
#define EVENTS_H

#include "utils.h"

namespace dynclamp {

class Entity;

typedef enum _event_type {
        SPIKE = 0
} EventType;

class Event
{
public:
        Event(EventType type, const Entity *sender);

        EventType type() const;
        const Entity* sender() const;

private:
        EventType m_type;
        const Entity *m_sender;
};

class SpikeEvent : public Event
{
public:
        SpikeEvent(const Entity *sender);
};

void EnqueueEvent(const Event *event);
void ProcessEvents();

} // namespace dynclamp

#endif

