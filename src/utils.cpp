#include <sstream>
#include <stdarg.h>
#include <stdio.h>
#include <assert.h>
#include "utils.h"
#include "events.h"
#include "entity.h"

#include <dlfcn.h>

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
        if (args.count("id") == 0) {
                *id = GetId();
        }
        else {
                *id = atoi(args["id"].c_str());
        }
        if (args.count("dt") == 0) {
                *dt = GetGlobalDt();
        }
        else {
                *dt = atof(args["dt"].c_str());
        }
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
        Logger(Critical, "Successfully opened library %s.\n", LIBNAME);

        sprintf(symbol, "%sFactory", entityName);

        addr = dlsym(library, symbol);
        if (addr == NULL) {
                Logger(Critical, "Unable to find symbol %s.\n", symbol);
                goto close_lib;
        }
        else {
                Logger(Critical, "Successfully found symbol %s.\n", symbol);
        }

        builder = (Factory) addr;
        entity = builder(args);

close_lib:
        if (dlclose(library) == 0) {
                Logger(Critical, "Successfully closed library %s.\n", LIBNAME);
        }
        else {
                Logger(Critical, "Unable to close library %s: %s.\n", LIBNAME, dlerror());
        }

        return entity;
}

} // namespace dynclamp

