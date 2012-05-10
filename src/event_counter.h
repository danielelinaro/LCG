#ifndef EVENT_COUNTER_H
#define EVENT_COUNTER_H

#include "entity.h"
#include "types.h"
#include "utils.h"

namespace dynclamp {

class EventCounter : public Entity {
public:
        EventCounter(uint maxCount, uint id = GetId());
        uint maxCount() const;
        uint count() const;
        void setMaxCount(uint count);
        virtual void handleEvent(const Event *event);
        virtual void step();
        double output() const;
        bool initialise();
        
private:
        void reset();

private:
        uint m_count, m_maxCount;
};

} // namespace dynclamp

/***
 *   FACTORY METHODS
 ***/
#ifdef __cplusplus
extern "C" {
#endif

dynclamp::Entity* EventCounterFactory(dictionary& args);
        
#ifdef __cplusplus
}
#endif

#endif

