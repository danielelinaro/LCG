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
#define LIBNAME "liblcg.dylib"
#elif defined(__linux__)
#define LIBNAME "liblcg.so"
#endif

namespace lcg
{

class Entity;

typedef enum
{
        All = 0, Debug, Info, Important, Critical
} LogLevel;

void SetLoggingLevel(LogLevel level);
LogLevel GetLoggingLevel();
#ifdef NDEBUG
#define Logger(level, fmt, ...) asm("nop")
#else
void Logger(LogLevel level, const char *fmt, ...);
#endif

void ResetIds();
uint GetId();

ullong GetRandomSeed();
uint GetIdFromDictionary(string_dict& args);
ullong GetSeedFromDictionary(string_dict& args);

bool CheckAndExtractValue(string_dict& dict, const std::string& key, std::string& value);
bool CheckAndExtractDouble(string_dict& dict, const std::string& key, double *value);
bool CheckAndExtractInteger(string_dict& dict, const std::string& key, int *value);
bool CheckAndExtractLong(string_dict& dict, const std::string& key, long *value);
bool CheckAndExtractLongLong(string_dict& dict, const std::string& key, long long *value);
bool CheckAndExtractUnsignedInteger(string_dict& dict, const std::string& key, unsigned int *value);
bool CheckAndExtractUnsignedLong(string_dict& dict, const std::string& key, unsigned long *value);
bool CheckAndExtractUnsignedLongLong(string_dict& dict, const std::string& key, unsigned long long *value);
bool CheckAndExtractBool(string_dict& dict, const std::string& key, bool *value);
void MakeFilename(char *filename, const char *extension);

Entity* EntityFactory(const char *name, string_dict& args);

/**
 * So far converts only Hz to seconds and vice versa.
 */
bool ConvertUnits(double x, double *y, const std::string& unitsIn, const std::string& unitsOut);

} // namespace lcg

#endif
