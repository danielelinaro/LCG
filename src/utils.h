#ifndef UTILS_H
#define UTILS_H

#include "types.h"

namespace dynclamp
{

typedef enum
{
        All = 0, Debug, Info, Critical
} LogLevel;

void SetLoggingLevel(LogLevel level);
void Logger(LogLevel level, const char *fmt, ...);

uint GetId();
void SetGlobalDt(double dt);
double GetGlobalDt();

} // namespace dynclamp

#endif
