#include <stdarg.h>
#include <stdio.h>
#include <assert.h>
#include "utils.h"
#include "events.h"
#include "entity.h"

#ifdef HAVE_CONFIG_H
#include "config.h"

#ifdef HAVE_LIBLXRT
#include <boost/thread.hpp>
#include <rtai_lxrt.h>
#include <rtai_shm.h>

/** Convert internal count units to milliseconds */
#define count2ms(t)     ((double) count2nano(t)*1e-6)
/** Convert milliseconds to internal count units*/
#define ms2count(t)     nano2count((t)*1e6)
/** Convert internal count units to seconds */
#define count2sec(t)     ((double) count2nano(t)*1e-9)
/** Convert seconds to internal count units */
#define sec2count(t)     nano2count((t)*1e9)

#define PRIORITY        1           /* 0 is the maximum */
#define STACK_SIZE      0           /* use default value */
#define MSG_SIZE        0           /* use default value */
#define DELAY           1           /* delay before starting the real-time loop (IN SECONDS) */

#endif // HAVE_LIBLXRT
#endif // HAVE_CONFIG_H


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

#ifdef HAVE_LIBLXRT

void RTSimulation(const std::vector<Entity*>& entities, double tend)
{
        RT_TASK *task;
        RTIME tickPeriod;
        RTIME currentTime, previousTime;
        int preambleIterations, flag, i;
        unsigned long taskName;
        double t0, dt = GetGlobalDt();
        size_t nEntities = entities.size();

        Logger(Info, "\n>>>>> DYNAMIC CLAMP THREAD STARTED <<<<<\n\n");

        Logger(Info, "Setting periodic mode.\n");
        rt_set_periodic_mode();

        Logger(Info, "Initialising task.\n");
        taskName = nam2num("hybrid_simulator");
        task = rt_task_init(taskName, PRIORITY, STACK_SIZE, MSG_SIZE);
        if(task == NULL) {
                Logger(Info, "Error: cannot initialise real-time task.\n");
                return;
        }

        Logger(Info, "Setting timer period.\n");
        tickPeriod = start_rt_timer(sec2count(dt));

        Logger(Info, "Switching to hard real time.\n");
        rt_make_hard_real_time();

        flag = rt_task_make_periodic_relative_ns(task, DELAY*1e9, dt*1e9);
        if(flag != 0) {
                Logger(Info, "Error while making the task periodic.\n");
                goto stopRT;
        }
        
        SetGlobalDt(count2sec(tickPeriod));
        Logger(Info, "The period is %g ms (f = %g Hz).\n", count2ms(tickPeriod), 1./count2sec(tickPeriod));

        // some sort of preamble
        previousTime = rt_get_time();
        preambleIterations = 0;
        while((currentTime = rt_get_time()) == previousTime) {
                previousTime = currentTime;
                preambleIterations++;
                rt_task_wait_period();
        }
        if (preambleIterations != 1)
                Logger(Info, "Performed %d iterations in the ``preamble''.\n", preambleIterations);
        else
                Logger(Info, "Performed 1 iteration in the ``preamble''.\n");

        while (GetGlobalTime() <= tend) {
                ProcessEvents();
                for (i=0; i<nEntities; i++)
                        entities[i]->readAndStoreInputs();
                IncreaseGlobalTime();
                for (i=0; i<nEntities; i++)
                        entities[i]->step();
                rt_task_wait_period();
        }

stopRT:
        Logger(Info, "Stopping the timer.\n");
        stop_rt_timer();

        Logger(Info, "Deleting the task.\n");
        rt_task_delete(task);

        Logger(Info, "\n>>>>> DYNAMIC CLAMP THREAD ENDED <<<<<\n\n");
}

#else

void NonRTSimulation(const std::vector<Entity*>& entities, double tend)
{
        size_t i, n = entities.size();
        ResetGlobalTime();
        while (GetGlobalTime() <= tend) {
                ProcessEvents();
                for (i=0; i<n; i++)
                        entities[i]->readAndStoreInputs();
                IncreaseGlobalTime();
                for (i=0; i<n; i++)
                        entities[i]->step();
        }
}

#endif // HAVE_LIBLXRT

void Simulate(const std::vector<Entity*>& entities, double tend)
{
#ifdef HAVE_LIBLXRT
        boost::thread thrd(RTSimulation, entities, tend);
#else
        boost::thread thrd(NonRTSimulation, entities, tend);
#endif // HAVE_LIBLXRT
        thrd.join();
}


} // namespace dynclamp

