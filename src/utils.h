#ifndef UTILS_H
#define UTILS_H

#include "types.h"

#define OK 0

namespace dynclamp
{

typedef enum
{
        All = 0, Debug, Info, Critical
} LogLevel;

void SetLoggingLevel(LogLevel level);
LogLevel GetLoggingLevel();
#ifdef NDEBUG
#define Logger(level, fmt, ...) asm("nop")
#else
void Logger(LogLevel level, const char *fmt, ...);
#endif

uint GetId();
void SetGlobalDt(double dt);
double GetGlobalDt();
void IncreaseGlobalTime();
double GetGlobalTime();

} // namespace dynclamp

#endif
