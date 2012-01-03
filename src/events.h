#ifndef EVENTS_H
#define EVENTS_H

#include "utils.h"

#define SPIKE_DELAY 2e-3

namespace dynclamp {

class DynamicalEntity;

typedef enum _event_type {
        SPIKE = 0
} EventType;

class Event
{
public:
        Event(EventType type, const DynamicalEntity *sender, double timeout);

        EventType type() const;
        double timeout() const;
        const DynamicalEntity* sender() const;
        bool hasExpired() const;

        void decreaseTimeout(double dt);

private:
        EventType m_type;
        double m_timeout;
        const DynamicalEntity *m_sender;
};

class SpikeEvent : public Event
{
public:
        SpikeEvent(const DynamicalEntity *sender, double timeout = SPIKE_DELAY);
};

//#ifdef __cplusplus
//extern "C" {
//#endif

void EnqueueEvent(const Event *event);
void ProcessEvents(double dt = GetGlobalDt());

//#ifdef __cplusplus
//}
//#endif

} // namespace dynclamp

#endif

