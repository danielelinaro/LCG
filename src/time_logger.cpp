#include "time_logger.h"
#include "utils.h"

namespace dynclamp {

TimeLogger::TimeLogger(uint id, double dt)
        : Entity(id, dt), m_time(0.0)
{}

void TimeLogger::step()
{
#ifdef HAVE_LIBCOMEDI
        m_time = count2sec(rt_get_time()) - GetGlobalTimeOffset();
#else
        m_time = GetGlobalTime();
#endif
}

double TimeLogger::output() const
{
        return m_time;
}

} // namespace dynclamp

