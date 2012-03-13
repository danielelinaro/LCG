#ifndef TIME_LOGGER_H
#define TIME_LOGGER_H

#include "entity.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

namespace dynclamp {

class TimeLogger : public Entity {
public:
        TimeLogger(uint id = GetId());
        virtual void step();
        virtual double output() const;
private:
        double m_time;
};

} // namespace dynclamp


#endif

