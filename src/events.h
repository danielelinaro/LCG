#ifndef EVENTS_H
#define EVENTS_H

#include "utils.h"

namespace dynclamp {

class Entity;

typedef enum _event_type {
        SPIKE = 0, TRIGGER, RESET
} EventType;

class Event
{
public:
        Event(EventType type, const Entity *sender);

        EventType type() const;
        const Entity* sender() const;

        double time() const;

private:
        EventType m_type;
        const Entity *m_sender;
        double m_time;
};

class SpikeEvent : public Event
{
public:
        SpikeEvent(const Entity *sender);
};

class TriggerEvent : public Event
{
public:
        TriggerEvent(const Entity *sender);
};

class ResetEvent : public Event
{
public:
        ResetEvent(const Entity *sender);
};

void EnqueueEvent(const Event *event);
void ProcessEvents();

} // namespace dynclamp

#endif

