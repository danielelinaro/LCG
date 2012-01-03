#include <stdarg.h>
#include <stdio.h>
#include <assert.h>
#include "utils.h"

namespace dynclamp
{

LogLevel verbosity = Info;
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
		vprintf(fmt, argp);
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

} // namespace dynclamp

