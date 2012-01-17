#ifndef UTILS_H
#define UTILS_H

#include <vector>
#include <boost/thread.hpp>
#include "types.h"

#define OK 0

namespace dynclamp
{

class Entity;

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
void IncreaseGlobalTime(double dt);
double GetGlobalTime();

void Simulate(const std::vector<Entity*>& entities, double tend);

} // namespace dynclamp

#endif
