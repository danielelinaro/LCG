#include <stdarg.h>
#include <stdio.h>
#include <assert.h>
#include "utils.h"
#include "events.h"
#include "entity.h"

namespace dynclamp
{

LogLevel verbosity = Info;
double globalT = 0.0;
double globalDt = 1.0 / 20e3;

void SetLoggingLevel(LogLevel level)
{
	verbosity = level;
#ifdef NDEBUG
        fprintf(stderr, "Logging has been disabled by defining NDEBUG.\n");
#endif
}

LogLevel GetLoggingLevel()
{
#ifdef NDEBUG
        fprintf(stderr, "Logging has been disabled by defining NDEBUG.\n");
#endif
        return verbosity;
}

#ifndef NDEBUG
void Logger(LogLevel level, const char *fmt, ...)
{
	if (level >= verbosity) {
		va_list argp;
		va_start(argp, fmt);
		vfprintf(stderr, fmt, argp);
		va_end(argp);
	}
}
#endif

uint GetId()
{
        static uint progressiveId = 0;
        progressiveId++;
        return progressiveId-1;
}

void SetGlobalDt(double dt)
{
        assert(dt > 0.0);
        globalDt = dt;
}

double GetGlobalDt()
{
        return globalDt;
}

double GetGlobalTime()
{
        return globalT;
}

void IncreaseGlobalTime()
{
        globalT += globalDt;
}

void ResetGlobalTime()
{
        globalT = 0.0;
}

#ifdef HAVE_RTAI_LXRT_H

void Simulate(const std::vector<Entity*>& entities, double tend)
{
}

#else

void Simulate(const std::vector<Entity*>& entities, double tend)
{
        size_t i, n = entities.size();
        ResetGlobalTime();
        while (GetGlobalTime() <= tend) {
                ProcessEvents();
                for (i=0; i<n; i++)
                        entities[i]->readAndStoreInputs();
                IncreaseGlobalTime();
                for (i=0; i<n; i++)
                        entities[i]->step();
        }
}

#endif


} // namespace dynclamp

