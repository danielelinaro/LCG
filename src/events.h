#ifndef EVENTS_H
#define EVENTS_H

#include "utils.h"
#include <string>

namespace dynclamp {

class Entity;

#define NUMBER_OF_EVENT_TYPES 4
typedef enum _event_type {
        SPIKE = 0, TRIGGER, RESET, TOGGLE
} EventType;
const std::string eventTypeNames[NUMBER_OF_EVENT_TYPES] = {"spike", "trigger", "reset", "toggle"};

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

class ToggleEvent : public Event
{
public:
        ToggleEvent(const Entity *sender);
};

void EnqueueEvent(const Event *event);
void ProcessEvents();

} // namespace dynclamp

#endif

