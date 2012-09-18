#ifndef EVENT_COUNTER_H
#define EVENT_COUNTER_H

#include "entity.h"
#include "types.h"
#include "utils.h"
#include "events.h"

namespace dynclamp {

class EventCounter : public Entity {
public:
        EventCounter(uint maxCount, bool autoReset = true,
                     EventType eventToCount = SPIKE, EventType eventToSend = TRIGGER,
                     uint id = GetId());
        uint maxCount() const;
        EventType eventToCount() const;
        EventType eventToSend() const;
        uint count() const;
        bool autoReset() const;
        void setMaxCount(uint count);
        void setAutoReset(bool autoReset);
        void setEventToCount(EventType eventToCount);
        void setEventToSend(EventType eventToSend);
        virtual void handleEvent(const Event *event);
        virtual void step();
        double output();
        bool initialise();
        
private:
        void reset();
        void dispatch();

private:
        uint m_count, m_maxCount;
        bool m_autoReset;
        EventType m_eventToCount;
        EventType m_eventToSend;
};

} // namespace dynclamp

/***
 *   FACTORY METHODS
 ***/
#ifdef __cplusplus
extern "C" {
#endif

dynclamp::Entity* EventCounterFactory(string_dict& args);
        
#ifdef __cplusplus
}
#endif

#endif

