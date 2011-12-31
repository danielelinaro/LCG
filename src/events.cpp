#include "events.h"

namespace dynclamp {

Event::Event(EventType type, double timeout, int senderId)
        : m_type(type), m_timeout(timeout), m_id(senderId)
{}

EventType Event::type() const
{
        return m_type;
}

double Event::timeout() const
{
        return m_timeout;
}

int Event::senderId() const
{
        return m_id;
}

bool Event::hasExpired() const
{
        return m_timeout <= 0;
}

void Event::decreaseTimeout(double dt)
{
        m_timeout -= dt;
}

SpikeEvent::SpikeEvent(double timeout, int senderId)
        : Event(SPIKE, timeout, senderId)
{}

} // namespace dynclamp

