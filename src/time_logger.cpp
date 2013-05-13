#include "time_logger.h"
#include "engine.h"

lcg::Entity* TimeLoggerFactory(string_dict& args)
{
        uint id = lcg::GetIdFromDictionary(args);
        return new lcg::TimeLogger(id);
}

namespace lcg {

TimeLogger::TimeLogger(uint id)
        : Entity(id)
{
        setName("TimeLogger");
        setUnits("s");
}

bool TimeLogger::initialise()
{
        m_time = 0.0;
        return true;
}

void TimeLogger::step()
{
#ifdef HAVE_LIBLXRT
        m_time = count2sec(rt_get_time()) - GetGlobalTimeOffset();
#else
        m_time = GetGlobalTime();
#endif
}

double TimeLogger::output()
{
        return m_time;
}

} // namespace lcg

