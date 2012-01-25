#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <dlfcn.h>
#include <sys/stat.h>

#include <sstream>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include "utils.h"
#include "events.h"
#include "entity.h"
#include "types.h"

namespace dynclamp
{

LogLevel verbosity = Info;
double globalT = 0.0;
double globalDt = 1.0 / 20e3;

void SetLoggingLevel(LogLevel level)
{
	verbosity = level;
#ifdef NDEBUG
        fprintf(stderr, "Logging has been disabled by defining NDEBUG.\n");
#endif
}

LogLevel GetLoggingLevel()
{
#ifdef NDEBUG
        fprintf(stderr, "Logging has been disabled by defining NDEBUG.\n");
#endif
        return verbosity;
}

#ifndef NDEBUG
void Logger(LogLevel level, const char *fmt, ...)
{
	if (level >= verbosity) {
		va_list argp;
		va_start(argp, fmt);
		vfprintf(stderr, fmt, argp);
		va_end(argp);
	}
}
#endif

uint GetId()
{
        static uint progressiveId = 0;
        progressiveId++;
        return progressiveId-1;
}

void SetGlobalDt(double dt)
{
        assert(dt > 0.0);
        globalDt = dt;
}

double GetGlobalDt()
{
        return globalDt;
}

void GetIdAndDtFromDictionary(dictionary& args, uint *id, double *dt)
{
        if (args.count("id") == 0)
                *id = GetId();
        else
                *id = atoi(args["id"].c_str());
        if (args.count("dt") == 0)
                *dt = GetGlobalDt();
        else
                *dt = atof(args["dt"].c_str());
}

void GetSeedFromDictionary(dictionary& args, ullong *seed)
{
        if (args.count("seed") == 0)
                *seed = SEED;
        else
                *seed = atoll(args["seed"].c_str());
}

bool CheckAndExtractValue(dictionary& dict, const std::string& key, std::string& value)
{
        if (dict.count(key)) {
                value = dict[key];
                return true;
        }
        Logger(Debug, "The required parameter [%s] is missing.\n", key.c_str());
        return false;
}

bool CheckAndExtractDouble(dictionary& dict, const std::string& key, double *value)
{
        std::string str;
        if (!CheckAndExtractValue(dict, key, str))
                return false;
        *value = atof(str.c_str());
        return true;
}

bool CheckAndExtractInteger(dictionary& dict, const std::string& key, int *value)
{
        std::string str;
        if (!CheckAndExtractValue(dict, key, str))
                return false;
        *value = atoi(str.c_str());
        return true;
}

bool CheckAndExtractUnsignedInteger(dictionary& dict, const std::string& key, uint *value)
{
        int i;
        if (!CheckAndExtractInteger(dict, key, &i) || i < 0)
                return false;
        *value = (uint) i;
        return true;
}

bool CheckAndExtractBool(dictionary& dict, const std::string& key, bool *value)
{
        std::string str;
        if (!CheckAndExtractValue(dict, key, str))
                return false;
        if (str.compare("true") == 0 || str.compare("TRUE") == 0 || str.compare("True") == 0)
                *value = true;
        else if (str.compare("false") == 0 || str.compare("FALSE") == 0 || str.compare("False") == 0)
                *value = false;
        else
                return false;
        return true;
}

void MakeFilename(char *filename, const char *extension)
{
        time_t rawTime;
        struct tm * timeInfo;
        int extensionLen, cnt;
        char *base;
        struct stat s;

        extensionLen = strlen(extension);

        base = new char[FILENAME_MAXLEN - extensionLen];

        time(&rawTime);
        timeInfo = localtime(&rawTime);

        sprintf(base, "%d%02d%02d%02d%02d%02d",
                timeInfo->tm_year+1900, timeInfo->tm_mon+1, timeInfo->tm_mday,
                timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_sec);

        sprintf(filename, "%s.%s", base, extension);

        cnt = 1;
        while (stat(filename, &s) == 0) {
                sprintf(filename, "%s-%02d.%s", base, cnt, extension);
                cnt++;
        }

        delete base;
}

double GetGlobalTime()
{
        return globalT;
}

void IncreaseGlobalTime()
{
        globalT += globalDt;
}

void IncreaseGlobalTime(double dt)
{
        globalT += dt;
}

void ResetGlobalTime()
{
        globalT = 0.0;
}

void ParseConfigurationFile(const std::string& filename, std::vector<Entity*>& entities)
{
}

Entity* EntityFactory(const char *entityName, dictionary& args)
{
        Entity *entity = NULL;
        Factory builder;
        void *library, *addr;
        char symbol[50] = {0};

        library = dlopen(LIBNAME, RTLD_LAZY);
        if (library == NULL) {
                Logger(Critical, "Unable to open library %s.\n", LIBNAME);
                return NULL;
        }
        Logger(Debug, "Successfully opened library %s.\n", LIBNAME);

        sprintf(symbol, "%sFactory", entityName);

        addr = dlsym(library, symbol);
        if (addr == NULL) {
                Logger(Critical, "Unable to find symbol %s.\n", symbol);
                goto close_lib;
        }
        else {
                Logger(Debug, "Successfully found symbol %s.\n", symbol);
        }

        builder = (Factory) addr;
        entity = builder(args);

close_lib:
        if (dlclose(library) == 0) {
                Logger(Debug, "Successfully closed library %s.\n", LIBNAME);
        }
        else {
                Logger(Critical, "Unable to close library %s: %s.\n", LIBNAME, dlerror());
        }

        return entity;
}

} // namespace dynclamp

