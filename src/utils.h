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

#define DYNCLAMP_VERSION 0.1

#define FILENAME_MAXLEN 1024

namespace dynclamp
{

class Entity;

typedef enum
{
        All = 0, Debug, Info, Important, Critical
} LogLevel;

struct CommandLineOptions {
        CommandLineOptions() : tend(-1), dt(0), iti(0), ibi(0), nTrials(0), nBatches(0),
        configFile(""), kernelFile(""), stimulusFiles() {}
        double tend, dt;
        useconds_t iti, ibi;
        uint nTrials, nBatches;
        std::string configFile, kernelFile;
        strings stimulusFiles;
};

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
bool CheckAndExtractLong(string_dict& dict, const std::string& key, int *value);
bool CheckAndExtractLongLong(string_dict& dict, const std::string& key, int *value);
bool CheckAndExtractUnsignedInteger(string_dict& dict, const std::string& key, uint *value);
bool CheckAndExtractUnsignedLong(string_dict& dict, const std::string& key, ullong *value);
bool CheckAndExtractUnsignedLongLong(string_dict& dict, const std::string& key, ullong *value);
bool CheckAndExtractBool(string_dict& dict, const std::string& key, bool *value);
void MakeFilename(char *filename, const char *extension);

bool ParseCommandLineOptions(int argc, char *argv[], CommandLineOptions *opt);
bool ParseConfigurationFile(const std::string& filename, std::vector<Entity*>& entities, double *tend, double *dt);

Entity* EntityFactory(const char *name, string_dict& args);

/**
 * So far converts only Hz to seconds and vice versa.
 */
bool ConvertUnits(double x, double *y, const std::string& unitsIn, const std::string& unitsOut);

} // namespace dynclamp

#endif
