#include <stdarg.h>
#include <stdio.h>
#include "utils.h"

namespace dynclamp
{

LogLevel verbosity = Info;

void SetLoggingLevel(LogLevel level)
{
	verbosity = level;
}

void Logger(LogLevel level, const char *fmt, ...)
{
	if (level >= verbosity) {
		va_list argp;
		va_start(argp, fmt);
		vprintf(fmt, argp);
		va_end(argp);
	}
}

} // namespace dynclamp

