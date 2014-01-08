/*=========================================================================
 *
 *   Program:     lcg
 *   Filename:    event_counter.h
 *
 *   Copyright (C) 2012,2013,2014 Daniele Linaro
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *   
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *   
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *=========================================================================*/

#ifndef EVENT_COUNTER_H
#define EVENT_COUNTER_H

#include "entity.h"
#include "types.h"
#include "utils.h"
#include "events.h"

namespace lcg {

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

} // namespace lcg

/***
 *   FACTORY METHODS
 ***/
#ifdef __cplusplus
extern "C" {
#endif

lcg::Entity* EventCounterFactory(string_dict& args);
        
#ifdef __cplusplus
}
#endif

#endif

