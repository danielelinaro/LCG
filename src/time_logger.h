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
        virtual bool initialise();
        virtual void step();
        virtual double output();
private:
        double m_time;
};

} // namespace dynclamp

/***
 *   FACTORY METHODS
 ***/
#ifdef __cplusplus
extern "C" {
#endif

dynclamp::Entity* TimeLoggerFactory(string_dict& args);
	
#ifdef __cplusplus
}
#endif

#endif // TIME_LOGGER_H

