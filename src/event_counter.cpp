#include "event_counter.h"
#include "engine.h"

dynclamp::Entity* EventCounterFactory(dictionary& args)
{
        uint id, maxCount;
        bool autoReset;

        id = dynclamp::GetIdFromDictionary(args);
        if (! dynclamp::CheckAndExtractUnsignedInteger(args, "maxCount", &maxCount)) {
                dynclamp::Logger(dynclamp::Critical, "Unable to build an EventCounter.\n");
                return NULL;
        }

        if (! dynclamp::CheckAndExtractBoolean(args, "autoReset", &autoReset)) {
                autoReset = true;
        }

        return new dynclamp::EventCounter(maxCount, autoReset, id);
}

namespace dynclamp {

EventCounter::EventCounter(uint maxCount, bool autoReset, uint id)
        : Entity(id), m_maxCount(maxCount), m_autoReset(autoReset)
{
        m_parameters.push_back(m_maxCount);
        m_parametersNames.push_back("maxCount");
        setName("EventCounter");
}

uint EventCounter::maxCount() const
{
        return m_maxCount;
}

uint EventCounter::count() const
{
        return m_count;
}

void EventCounter::setMaxCount(uint maxCount)
{
        m_maxCount = maxCount;
}

void EventCounter::handleEvent(const Event *event)
{
        switch(event->type()) {
        case SPIKE:
                m_count++;
                if (m_count == m_maxCount) {
                        Logger(Debug, "Received %d spikes at t = %g sec.\n", m_maxCount, GetGlobalTime());
                        emitEvent(new TriggerEvent(this));
                        if (m_autoReset)
                        reset();
                }
                break;
        case RESET:
                reset();
                break;
        default:
                Logger(Important, "EventCounter: unknown event type.\n");
        }
}

void EventCounter::step()
{}

double EventCounter::output() const
{
        return 0.0;
}

bool EventCounter::initialise()
{
        m_count = 0;
}

void EventCounter::reset()
{
        m_count = 0;
}

} // namespace dynclamp

