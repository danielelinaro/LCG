#ifndef TIME_LOGGER_H
#define TIME_LOGGER_H

#include "entity.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

namespace lcg {

class TimeLogger : public Entity {
public:
        TimeLogger(uint id = GetId());
        virtual bool initialise();
        virtual void step();
        virtual double output();
private:
        double m_time;
};

} // namespace lcg

/***
 *   FACTORY METHODS
 ***/
#ifdef __cplusplus
extern "C" {
#endif

lcg::Entity* TimeLoggerFactory(string_dict& args);
	
#ifdef __cplusplus
}
#endif

#endif // TIME_LOGGER_H

