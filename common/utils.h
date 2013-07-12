#ifndef UTILS_H
#define UTILS_H

#include <vector>
#include "types.h"
#include "common.h"
#include "h5rec.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

namespace lcg
{

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

bool CheckAndExtractValue(string_dict& dict, const char *key, std::string& value);
bool CheckAndExtractDouble(string_dict& dict, const char *key, double *value);
bool CheckAndExtractInteger(string_dict& dict, const char *key, int *value);
bool CheckAndExtractLong(string_dict& dict, const char *key, long *value);
bool CheckAndExtractLongLong(string_dict& dict, const char *key, long long *value);
bool CheckAndExtractUnsignedInteger(string_dict& dict, const char *key, unsigned int *value);
bool CheckAndExtractUnsignedLong(string_dict& dict, const char *key, unsigned long *value);
bool CheckAndExtractUnsignedLongLong(string_dict& dict, const char *key, unsigned long long *value);
bool CheckAndExtractBool(string_dict& dict, const char *key, bool *value);
void MakeFilename(char *filename, const char *extension);

/**
 * So far converts only Hz to seconds and vice versa.
 */
bool ConvertUnits(double x, double *y, const char *unitsIn, const char *unitsOut);

double SetGlobalDt(double dt);

extern double globalT;
extern double globalDt;
extern double runTime;
#define GetGlobalDt() globalDt
#define GetGlobalTime() globalT
#define IncreaseGlobalTime() (globalT += globalDt)
#define ResetGlobalTime()  (globalT = 0.0)
#define SetRunTime(tend) (runTime = tend)
#define GetRunTime() runTime

////// SIGNAL HANDLING CODE - START /////
extern bool programRun;
extern bool trialRun;
#define KILL_PROGRAM() !programRun
#define TERMINATE_TRIAL() !trialRun
bool SetupSignalCatching();
////// SIGNAL HANDLING CODE - END /////

void SetTrialRun(bool value);
/*! Stops the execution of the program (like issuing a SIGINT signal). */
void KillProgram();
/*! Stops the execution of the current trial. */
void TerminateTrial();

void StartCommentsReaderThread(H5RecorderCore *rec);
void StopCommentsReaderThread();

} // namespace lcg

#endif
