#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <signal.h>
#include <math.h>

#include <sstream>
#include <algorithm>
#include <iostream>

#include "utils.h"

/* colors */
#define ESC ''
#define RED "[31m"
#define GREEN "[32m"
#define YELLOW "[33m"
#define NORMAL "[00m"

namespace lcg
{

double globalT;
double globalDt = SetGlobalDt(1.0/20e3);
double runTime;

////// COMMENTS HANDLING CODE - START //////

bool readingComment = false;
bool commentsThreadRunning = false;
pthread_t commentsThread;
pthread_mutex_t commentsMutex;
pthread_cond_t commentsCV;
std::vector< std::pair<std::string,time_t> > comments;

const std::vector< std::pair<std::string,time_t> >* GetComments()
{
        while (commentsThreadRunning)
                ;
        return &comments;
}

void* CommentsReader(void *)
{
        char c;
        int old;
        time_t now;
        std::string msg;
        // this is superfluous: PTHREAD_CANCEL_ENABLE is the default for new threads
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &old); 
        pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &old);
        comments.clear();
        Logger(Debug, "CommentsReader started.\n");
        while (!TERMINATE_TRIAL()) {
                if ((c = getchar()) == 'c') {
                        pthread_mutex_lock(&commentsMutex);
                        readingComment = true;
                        getchar(); // remove the newline character
                        now = time(NULL);
                        Logger(Info, "Enter comment: ");
                        std::getline(std::cin, msg);
                        comments.push_back(std::make_pair(msg,now));
                        readingComment = false;
                        pthread_mutex_unlock(&commentsMutex);
                }
                pthread_cond_signal(&commentsCV);
        }
        Logger(Debug, "CommentsReader ended.\n");
        pthread_exit(NULL);
}

void StartCommentsReaderThread()
{
        commentsThreadRunning = false;
        int err = pthread_mutex_init(&commentsMutex, NULL);
        if (err) {
                Logger(Critical, "pthread_mutex_init: %s\n", strerror(err));
                return;
        }
        err = pthread_cond_init(&commentsCV, NULL);
        if (err) {
                Logger(Critical, "pthread_cond_init: %s\n", strerror(err));
                return;
        }
        err = pthread_create(&commentsThread, NULL, CommentsReader, NULL);
        if (err) {
                Logger(Critical, "pthread_create: %s\n", strerror(err));
                return;
        }
        else {
                commentsThreadRunning = true;
                Logger(Debug, "Comments reader thread started.\n");
        }
}

void StopCommentsReaderThread()
{
        Logger(Debug, "Stopping comments reader thread.\n");
        pthread_mutex_lock(&commentsMutex);
        while (readingComment)
                pthread_cond_wait(&commentsCV, &commentsMutex);
        pthread_mutex_unlock(&commentsMutex);
        if (pthread_cancel(commentsThread) == 0)
                Logger(Debug, "Comments reader thread cancelled.\n");
        else
                Logger(Debug, "Unable to cancel comments reader thread.\n");
        commentsThreadRunning = false;
        pthread_mutex_destroy(&commentsMutex);
        pthread_cond_destroy(&commentsCV);
}

////// COMMENTS HANDLING CODE - END //////

////// SIGNAL HANDLING CODE - START //////

bool programRun = true; 
bool trialRun = false; 
pthread_mutex_t programRunMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t trialRunMutex = PTHREAD_MUTEX_INITIALIZER;

void TerminationHandler(int signum)
{
        if (signum == SIGINT || signum == SIGHUP) {
                Logger(Critical, "Terminating the program.\n");
                KillProgram();
        }
}

bool SetupSignalCatching()
{
        struct sigaction oldAction, newAction;
        int i, sig[] = {SIGINT, SIGHUP, -1};
        // set up the structure to specify the new action.
        newAction.sa_handler = TerminationHandler;
        sigemptyset(&newAction.sa_mask);
        newAction.sa_flags = 0;
        i = 0;
        while (sig[i] != -1) {
                if (sigaction(sig[i], NULL, &oldAction) != 0) {
                        perror("Error on sigaction:");
                        return false;
                }
                if (oldAction.sa_handler != SIG_IGN) {
                        if (sigaction(sig[i], &newAction, NULL) != 0) {
                             perror("Error on sigaction:");
                             return false;
                        }
                }
                i++;
        }
        return true;
}

////// SIGNAL HANDLING CODE - END //////

void SetTrialRun(bool value)
{
        Logger(Debug, "SetTrialRun(%s)\n", value ? "True" : "False");
        pthread_mutex_lock(&trialRunMutex);
        trialRun = value;
        pthread_mutex_unlock(&trialRunMutex);
}

void KillProgram()
{
        Logger(Debug, "KillProgram()\n");
        TerminateTrial();
        pthread_mutex_lock(&programRunMutex);
        programRun = false;
}

void TerminateTrial()
{
        Logger(Debug, "TerminateTrial()\n");
        SetTrialRun(false);
}

double SetGlobalDt(double dt)
{
        assert(dt > 0.0);
        globalDt = dt;

#ifdef HAVE_LIBLXRT
        Logger(Debug, "Starting RT timer.\n");
        RTIME period = start_rt_timer(sec2count(dt));
        realtimeDt = count2sec(period);
        Logger(Debug, "The real time period is %g ms (f = %g Hz).\n", realtimeDt*1e3, 1./realtimeDt);
#ifndef NO_STOP_RT_TIMER
        Logger(Debug, "Stopping RT timer.\n");
        stop_rt_timer();
#endif // NO_STOP_RT_TIMER
#endif // HAVE_LIBLXRT

#ifdef HAVE_LIBANALOGY
        Logger(Debug, "The real time period is %g ms (f = %g Hz).\n", globalDt*1e3, 1./globalDt);
#endif // HAVE_LIBANALOGY

#ifdef HAVE_LIBRT
        struct timespec ts;
        clock_getres(CLOCK_REALTIME, &ts);
        if (ts.tv_nsec != 1) {
                long cycles = round(globalDt * NSEC_PER_SEC / ts.tv_nsec);
                globalDt = cycles * ts.tv_nsec / NSEC_PER_SEC;
        }
        Logger(Debug, "The real time period is %g ms (f = %g Hz).\n", globalDt*1e3, 1./globalDt);
#endif // HAVE_LIBRT

        assert(globalDt > 0.0);
        return globalDt;
}

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

std::string &LeftTrim(std::string &s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
        return s;
}
std::string &RightTrim(std::string &s) {
        s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
        return s;
}
std::string &Trim(std::string &s) {
        return LeftTrim(RightTrim(s));
}

} // namespace lcg

