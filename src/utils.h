#ifndef UTILS_H
#define UTILS_H

#include <vector>
#include "types.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_LIBLXRT
#include <rtai_lxrt.h>

/** Convert internal count units to milliseconds */
#define count2ms(t)     ((double) count2nano(t)*1e-6)
/** Convert milliseconds to internal count units*/
#define ms2count(t)     nano2count((t)*1e6)
/** Convert internal count units to seconds */
#define count2sec(t)     ((double) count2nano(t)*1e-9)
/** Convert seconds to internal count units */
#define sec2count(t)     nano2count((t)*1e9)

#endif // HAVE_LIBLXRT

#if defined(__APPLE__)
#define LIBNAME "libdynclamp.dylib"
#elif defined(__linux__)
#define LIBNAME "libdynclamp.so"
#endif

#define OK 0
#define FILENAME_MAXLEN 30

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

void ResetGlobalTime();
void IncreaseGlobalTime();
void IncreaseGlobalTime(double dt);
double GetGlobalTime();
void SetGlobalDt(double dt);
double GetGlobalDt();
#ifdef HAVE_LIBLXRT
void SetGlobalTimeOffset();
double GetGlobalTimeOffset();
#endif // HAVE_LIBLXRT

ullong GetRandomSeed();

void GetIdAndDtFromDictionary(dictionary& args, uint *id, double *dt);
void GetSeedFromDictionary(dictionary& args, ullong *seed);
bool CheckAndExtractValue(dictionary& dict, const std::string& key, std::string& value);
bool CheckAndExtractDouble(dictionary& dict, const std::string& key, double *value);
bool CheckAndExtractInteger(dictionary& dict, const std::string& key, int *value);
bool CheckAndExtractLong(dictionary& dict, const std::string& key, int *value);
bool CheckAndExtractLongLong(dictionary& dict, const std::string& key, int *value);
bool CheckAndExtractUnsignedInteger(dictionary& dict, const std::string& key, uint *value);
bool CheckAndExtractUnsignedLong(dictionary& dict, const std::string& key, ullong *value);
bool CheckAndExtractUnsignedLongLong(dictionary& dict, const std::string& key, ullong *value);
bool CheckAndExtractBool(dictionary& dict, const std::string& key, bool *value);
void MakeFilename(char *filename, const char *extension);

bool ParseConfigurationFile(const std::string& filename, std::vector<Entity*>& entities);

Entity* EntityFactory(const char *name, dictionary& args);

} // namespace dynclamp

#endif
