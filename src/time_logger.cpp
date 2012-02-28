#include "time_logger.h"

namespace dynclamp {

TimeLogger::TimeLogger(uint id, double dt)
        : Entity(id, dt)
{}

void TimeLogger::step()
{
}

double TimeLogger::output() const
{
        return m_time;
}

} // namespace dynclamp

