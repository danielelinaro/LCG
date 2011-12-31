#ifndef EVENTS_H
#define EVENTS_H

namespace dynclamp {

typedef enum _event_type {
        SPIKE = 0
} EventType;

class Event
{
public:
        Event(EventType type, double timeout, int senderId);

        EventType type() const;
        double timeout() const;
        int senderId() const;
        bool hasExpired() const;

        void decreaseTimeout(double dt);

private:
        EventType m_type;
        double m_timeout;
        int m_id;
};

class SpikeEvent : public Event
{
public:
        SpikeEvent(double timeout, int senderId);
};

} // namespace dynclamp

#endif

