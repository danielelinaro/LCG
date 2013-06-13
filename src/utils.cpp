#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <strings.h>
#include <dlfcn.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>

#include <sstream>
#include <algorithm>
#include <vector>

#include "utils.h"
#include "events.h"
#include "entity.h"
#include "types.h"
#include "engine.h"
#include "common.h"

/* colors */
#define ESC ''
#define RED "[31m"
#define GREEN "[32m"
#define YELLOW "[33m"
#define NORMAL "[00m"

namespace lcg
{

LogLevel verbosity = Info;
uint progressiveId = -1;

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
                switch (level) {
                case Critical:
                        fprintf(stderr, "%c%s", ESC, RED);
                        break;
                case Important:
                        fprintf(stderr, "%c%s", ESC, YELLOW);
                        break;
                }
		vfprintf(stderr, fmt, argp);
                fprintf(stderr, "%c%s", ESC, NORMAL);
		va_end(argp);
	}
}
#endif

void ResetIds()
{
        progressiveId = -1;
}

uint GetId()
{
        progressiveId++;
        return progressiveId;
}

uint GetIdFromDictionary(string_dict& args)
{
        uint id;
        if (args.count("id") == 0) {
                id = GetId();
        }
        else {
                std::stringstream ss(args["id"]);
                ss >> id;
        }
        return id;
}

ullong GetRandomSeed()
{
        ullong seed;
        int fd = open("/dev/urandom",O_RDONLY);
        if (fd == -1) {
                seed = time(NULL);
                Logger(Debug, "seed = time(NULL) = 0x%llx.\n", seed);
                goto endGetRandomSeed;
        }
        if (read(fd, (void *) &seed, sizeof(ullong)) != sizeof(ullong)) {
                seed = time(NULL);
                Logger(Debug, "seed = time(NULL) = 0x%llx.\n", seed);
        }
        else {
                seed &= 0xffffffff;
                Logger(Debug, "seed = 0x%llx\n", seed);
        }
        if (close(fd) != 0)
                perror("Error while closing /dev/urandom: ");
endGetRandomSeed:
        return seed;
}

ullong GetSeedFromDictionary(string_dict& args)
{
        ullong s;
        if (args.count("seed") == 0 || (s = atoll(args["seed"].c_str())) == -1)
                s = GetRandomSeed();
        return s;
}

bool CheckAndExtractValue(string_dict& dict, const char *key, std::string& value)
{
        if (dict.count(key)) {
                value = dict[key];
                return true;
        }
        Logger(Debug, "The requested key [%s] is missing.\n", key);
        return false;
}

bool CheckAndExtractDouble(string_dict& dict, const char *key, double *value)
{
        std::string str;
        if (!CheckAndExtractValue(dict, key, str))
                return false;
        *value = atof(str.c_str());
        return true;
}

bool CheckAndExtractInteger(string_dict& dict, const char *key, int *value)
{
        std::string str;
        if (!CheckAndExtractValue(dict, key, str))
                return false;
        *value = atoi(str.c_str());
        return true;
}

bool CheckAndExtractLong(string_dict& dict, const char *key, long *value)
{
        std::string str;
        if (!CheckAndExtractValue(dict, key, str))
                return false;
        *value = atol(str.c_str());
        return true;
}

bool CheckAndExtractLongLong(string_dict& dict, const char *key, long long *value)
{
        std::string str;
        if (!CheckAndExtractValue(dict, key, str))
                return false;
        *value = atoll(str.c_str());
        return true;
}

bool CheckAndExtractUnsignedInteger(string_dict& dict, const char *key, unsigned int *value)
{
        int i;
        if (!CheckAndExtractInteger(dict, key, &i) || i < 0)
                return false;
        *value = (unsigned int) i;
        return true;
}

bool CheckAndExtractUnsignedLong(string_dict& dict, const char *key, unsigned long *value)
{
        long l;
        if (!CheckAndExtractLong(dict, key, &l) || l < 0)
                return false;
        *value = (unsigned long) l;
        return true;
}

bool CheckAndExtractUnsignedLongLong(string_dict& dict, const char *key, unsigned long long *value)
{
        long long l;
        if (!CheckAndExtractLongLong(dict, key, &l) || l < 0)
                return false;
        *value = (unsigned long long) l;
        return true;
}

bool CheckAndExtractBool(string_dict& dict, const char *key, bool *value)
{
        std::string str;
        if (!CheckAndExtractValue(dict, key, str))
                return false;
        if (strncasecmp(str.c_str(), "true", 5) == 0)
                *value = true;
        else if (strncasecmp(str.c_str(), "false", 6) == 0)
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
        struct stat buf;

        extensionLen = strlen(extension);

        base = new char[FILENAME_MAXLEN - extensionLen];

        time(&rawTime);
        timeInfo = localtime(&rawTime);

        sprintf(base, "%d%02d%02d%02d%02d%02d",
                timeInfo->tm_year+1900, timeInfo->tm_mon+1, timeInfo->tm_mday,
                timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_sec);

        sprintf(filename, "%s.%s", base, extension);

        cnt = 1;
        while (stat(filename, &buf) != -1) {
                sprintf(filename, "%s-%02d.%s", base, cnt, extension);
                cnt++;
        }

        delete base;
}

Entity* EntityFactory(const char *entityName, string_dict& args)
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

bool ConvertUnits(double x, double *y, const char *unitsIn, const char *unitsOut)
{
        if (strcasecmp(unitsIn, unitsOut) == 0) {
                *y = x;
                return true;
        }

        if ((strncasecmp(unitsIn, "s", 2) == 0 && strncasecmp(unitsOut, "Hz", 3) == 0) ||
            (strncasecmp(unitsIn, "Hz", 3) == 0 && strncasecmp(unitsOut, "s", 2) == 0)) {
                *y = 1.0 / x;
                return true;
        }

        return false;
}
} // namespace lcg

