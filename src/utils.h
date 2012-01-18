#ifndef UTILS_H
#define UTILS_H

#include <vector>
#include "types.h"

#if defined(__APPLE__)
#define LIBNAME "libdynclamp.dylib"
#elif defined(__linux__)
#define LIBNAME "libdynclamp.so"
#endif

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

void GetIdAndDtFromDictionary(dictionary& args, uint *id, double *dt);
bool CheckAndExtractValue(dictionary& dict, const std::string& key, const std::string& value);
bool CheckAndExtractDouble(dictionary& dict, const std::string& key, double *value);
bool CheckAndExtractInteger(dictionary& dict, const std::string& key, int *value);

void ResetGlobalTime();
void IncreaseGlobalTime();
void IncreaseGlobalTime(double dt);
double GetGlobalTime();

Entity* EntityFactory(const char *name, dictionary& args);

} // namespace dynclamp

#endif
