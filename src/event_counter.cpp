#include "event_counter.h"
#include "engine.h"
#include "events.h"
#include <boost/algorithm/string.hpp>

dynclamp::Entity* EventCounterFactory(dictionary& args)
{
        uint id, maxCount;
        bool autoReset;
        std::string eventStr;
        uint event;

        id = dynclamp::GetIdFromDictionary(args);
        if (! dynclamp::CheckAndExtractUnsignedInteger(args, "maxCount", &maxCount)) {
                dynclamp::Logger(dynclamp::Critical, "Unable to build an EventCounter.\n");
                return NULL;
        }

        if (! dynclamp::CheckAndExtractBool(args, "autoReset", &autoReset)) {
                autoReset = true;
        }

        if (dynclamp::CheckAndExtractValue(args, "event", eventStr)) {
                int i=0;
                while(i < NUMBER_OF_EVENT_TYPES && !boost::iequals(eventStr, dynclamp::eventTypeNames[i]))
                    i++;
                if (i == NUMBER_OF_EVENT_TYPES) {
                    dynclamp::Logger(dynclamp::Critical, "Unknown event type [%s].\n", eventStr.c_str());
                    return NULL;
                }
                event = i;
        }
        else {
                event = dynclamp::TRIGGER;
        }
        return new dynclamp::EventCounter(maxCount, autoReset, event, id);
}

namespace dynclamp {

EventCounter::EventCounter(uint maxCount, bool autoReset,uint event, uint id)
        : Entity(id), m_maxCount(maxCount), m_autoReset(autoReset), m_event(event)
{
        m_parameters.push_back(m_maxCount);
        m_parametersNames.push_back("maxCount");
        setName("EventCounter");
}

uint EventCounter::maxCount() const
{
        return m_maxCount;
}
bool EventCounter::autoReset() const
{
        return m_autoReset;
}
uint EventCounter::event() const
{
        return m_event;
}

uint EventCounter::count() const
{
        return m_count;
}

void EventCounter::setMaxCount(uint maxCount)
{
        m_maxCount = maxCount;
}
void EventCounter::setAutoReset(bool autoReset)
{
        m_autoReset = autoReset;
}
void EventCounter::setEvent(uint event)
{
        m_event = event;
}
void EventCounter::handleEvent(const Event *event)
{
        switch(event->type()) {
        case SPIKE:
                m_count++;
                if (m_count == m_maxCount) {
                        Logger(Debug, "Received %d spikes at t = %g sec.\n", m_maxCount, GetGlobalTime());
                        dispatch(); 
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
void EventCounter::dispatch()
{
    switch(event()) {
        case TRIGGER: 
            emitEvent(new TriggerEvent(this));
            break;
        case SPIKE:
            emitEvent(new SpikeEvent(this));
            break;
        case RESET:
            emitEvent(new ResetEvent(this));
            break;
        case TOGGLE:
            emitEvent(new ToggleEvent(this));
            break;
        default:
            Logger(Important, "EventCounter: Can't send event.\n");
            break;
    }
}

} // namespace dynclamp

