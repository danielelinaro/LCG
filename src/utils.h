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

class CommandLineOptions {
public:
        CommandLineOptions() : tend(-1), dt(0), iti(0), ibi(0), nTrials(0), nBatches(0),
        configFile(""), kernelFile(""), stimulusFiles() {}

public:
        double tend, dt;
        useconds_t iti, ibi;
        uint nTrials, nBatches;
        std::string configFile, kernelFile;
        std::vector<std::string> stimulusFiles;
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
uint GetIdFromDictionary(dictionary& args);
ullong GetSeedFromDictionary(dictionary& args);

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

bool ParseCommandLineOptions(int argc, char *argv[], CommandLineOptions *opt);
bool ParseConfigurationFile(const std::string& filename, std::vector<Entity*>& entities, double *tend, double *dt);

Entity* EntityFactory(const char *name, dictionary& args);

} // namespace dynclamp

#endif
