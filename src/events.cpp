#include "events.h"
#include "dynamical_entity.h"
#include "thread_safe_queue.h"

namespace dynclamp {

ThreadSafeQueue<Event*> eventsQueue;

void EnqueueEvent(Event *event)
{
        eventsQueue.push_back(event);
        Logger(Debug, "Enqueued event sent from entity #%d.\n", event->sender()->id());
}

void ProcessEvents(double dt)
{
        Event *event;
        uint i, j, nEvents, nPost;
        nEvents = eventsQueue.size();
        Logger(Debug, "There are %d events in the queue.\n", nEvents);
        for (i=0; i<nEvents; i++) {
                event = eventsQueue.pop_front();
                if (event->hasExpired()) {
                        const std::vector<DynamicalEntity*>& post = event->sender()->post();
                        nPost = post.size();
                        for (j=0; j<nPost; j++)
                                post[j]->handleEvent(event);
                        delete event;
                        Logger(Debug, "Deleted event sent from entity #%d.\n", event->sender()->id());
                }
                else {
                        event->decreaseTimeout(dt);
                        EnqueueEvent(event);
                }
        }
}

Event::Event(EventType type, const DynamicalEntity *sender, double timeout)
        : m_type(type), m_timeout(timeout), m_sender(sender)
{}

EventType Event::type() const
{
        return m_type;
}

double Event::timeout() const
{
        return m_timeout;
}

const DynamicalEntity* Event::sender() const
{
        return m_sender;
}

bool Event::hasExpired() const
{
        return m_timeout <= 0;
}

void Event::decreaseTimeout(double dt)
{
        m_timeout -= dt;
}

SpikeEvent::SpikeEvent(const DynamicalEntity *sender, double timeout)
        : Event(SPIKE, sender, timeout)
{}

} // namespace dynclamp

