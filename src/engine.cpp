#include <boost/thread.hpp>
#include <stdio.h>
#include "engine.h"
#include "entity.h"
#include "utils.h"

#ifdef HAVE_LIBLXRT
#include <rtai_lxrt.h>
#include <rtai_shm.h>

#define PRIORITY        0           /* 0 is the maximum */
#define STACK_SIZE      0           /* use default value */
#define MSG_SIZE        0           /* use default value */
#endif // HAVE_LIBLXRT

#ifdef HAVE_LIBRT
#include <errno.h>
#endif

namespace dynclamp {

double globalT;
double globalDt = SetGlobalDt(1.0/20e3);
#ifdef REAL_TIME_ENGINE
double globalTimeOffset = 0.0;
#endif
#ifdef HAVE_LIBLXRT
double realtimeDt;
#endif

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

#ifdef HAVE_LIBRT
        struct timespec ts;
        clock_getres(CLOCK_REALTIME, &ts);
        if (ts.tv_nsec != 1) {
                long cycles = round(globalDt * NSEC_PER_SEC / ts.tv_nsec);
                globalDt = cycles * ts.tv_nsec / NSEC_PER_SEC;
        }
        Logger(Debug, "The real time period is %g ms (f = %g Hz).\n", globalDt*1e3, 1./globalDt);
#endif
        return globalDt;
}


#if defined(HAVE_LIBLXRT)

void RTSimulation(const std::vector<Entity*>& entities, double tend)
{
        RT_TASK *task;
        RTIME tickPeriod;
        RTIME currentTime, previousTime;
        int preambleIterations, flag, i;
        unsigned long taskName;
        size_t nEntities = entities.size();

        Logger(Info, "\n>>>>> DYNAMIC CLAMP THREAD STARTED <<<<<\n\n");

#ifndef NO_STOP_RT_TIMER
        Logger(Info, "Setting periodic mode.\n");
        rt_set_periodic_mode();
#endif

        Logger(Info, "Initialising task.\n");
        taskName = nam2num("hybrid_simulator");
        task = rt_task_init(taskName, RT_SCHED_HIGHEST_PRIORITY+1, STACK_SIZE, MSG_SIZE);
        if(task == NULL) {
                Logger(Critical, "Error: cannot initialise real-time task.\n");
                return;
        }

        Logger(Info, "Setting timer period.\n");
        tickPeriod = start_rt_timer(sec2count(globalDt));
        
        Logger(Info, "The period is %.6g ms (f = %.6g Hz).\n", count2ms(tickPeriod), 1./count2sec(tickPeriod));

        Logger(Info, "Switching to hard real time.\n");
        rt_make_hard_real_time();

        Logger(Info, "Making the task periodic.\n");
        flag = rt_task_make_periodic_relative_ns(task, count2nano(5*tickPeriod), count2nano(tickPeriod));
        if(flag != 0) {
                Logger(Critical, "Error while making the task periodic.\n");
                goto stopRT;
        }
        
        // some sort of preamble
        currentTime = rt_get_time();
        preambleIterations = 0;
        do {
                previousTime = currentTime;
                rt_task_wait_period();
                currentTime = rt_get_time();
                preambleIterations++;
        } while (currentTime == previousTime);
        Logger(Debug, "Performed %d iteration%s in the ``preamble''.\n",
                preambleIterations, (preambleIterations == 1 ? "" : "s"));

        SetGlobalTimeOffset();
        ResetGlobalTime();
        for (uint i=0; i<entities.size(); i++)
                entities[i]->initialise();
        while (GetGlobalTime() <= tend) {
                ProcessEvents();
                for (i=0; i<nEntities; i++)
                        entities[i]->readAndStoreInputs();
                IncreaseGlobalTime();
                for (i=0; i<nEntities; i++)
                        entities[i]->step();
                rt_task_wait_period();
                //Logger(Info, "%g\n", GetGlobalTime());
        }

stopRT:
#ifndef NO_STOP_RT_TIMER
        Logger(Info, "Stopping the timer.\n");
        stop_rt_timer();
#endif

        Logger(Info, "Deleting the task.\n");
        rt_task_delete(task);

        Logger(Info, "\n>>>>> DYNAMIC CLAMP THREAD ENDED <<<<<\n\n");
}

#elif defined(HAVE_LIBRT)

static inline void tsnorm(struct timespec *ts)
{
	while (ts->tv_nsec >= NSEC_PER_SEC) {
		ts->tv_nsec -= NSEC_PER_SEC;
		ts->tv_sec++;
	}
}

static bool CheckPrivileges()
{
        int policy = sched_getscheduler(0);
        struct sched_param param, old_param;

        /* 
         * if we're already running a realtime scheduler
         * then we *should* be able to change things later
         */
        if (policy == SCHED_FIFO || policy == SCHED_RR)
                return true;

        /* first get the current parameters */
        if (sched_getparam(0, &old_param)) {
                Logger(Critical, "Unable to get scheduler parameters.\n");
                return false;
        }
        param = old_param;

        /* try to change to SCHEDULER */
        param.sched_priority = 1;
        if (sched_setscheduler(0, SCHEDULER, &param)) {
                Logger(Critical, "Unable to change scheduling policy!\n");
                Logger(Critical, "Either run as root or join realtime group.\n");
                return false;
        }

        /* we're good; change back and return success */
        return sched_setscheduler(0, policy, &old_param) == 0;
}

static inline int64_t calcdiff_ns(struct timespec t1, struct timespec t2)
{
        int64_t diff;
        diff = NSEC_PER_SEC * (int64_t)((int) t1.tv_sec - (int) t2.tv_sec);
        diff += ((int) t1.tv_nsec - (int) t2.tv_nsec);
        return diff;
}

void RTSimulation(const std::vector<Entity*>& entities, double tend)
{
        int priority, flag, i;
        size_t nEntities = entities.size();
        struct timespec now, next, interval;
        struct sched_param schedp;

        priority = sched_get_priority_max(SCHEDULER);
        if (priority < 0) {
                Logger(Critical, "Unable to get maximum priority.\n");
                perror("sched_get_priority_max");
                return;
        }
        Logger(Debug, "The maximum priority is #%d.\n", priority);

	memset(&schedp, 0, sizeof(schedp));
	schedp.sched_priority = priority;
	flag = sched_setscheduler(0, SCHEDULER, &schedp);
        if (flag != 0){
		Logger(Critical, "Unable to set maximum priority.\n");
		return; 
	}
        Logger(Info, "Successfully set maximum priority.\n");

        // Reset simulation time
        ResetGlobalTime();

        // Initialise all entities
        for (uint i=0; i<entities.size(); i++)
                entities[i]->initialise();
        Logger(Info, "Initialised all entities.\n");
        
	// Get current time
	flag = clock_gettime(CLOCK_REALTIME, &now);
        if (flag < 0) {
                Logger(Critical, "Unable to get time from the system.\n");
                perror("clock_gettime");
                return;
        }

	interval.tv_sec = 0;
	interval.tv_nsec = GetGlobalDt() * NSEC_PER_SEC;

        // Compute the time at which the thread will have to resume
        next = now;
	next.tv_sec += interval.tv_sec;
	next.tv_nsec += interval.tv_nsec;
	tsnorm(&next);

        // Set the time offset, i.e. the absolute time of the beginning of the simulation
        SetGlobalTimeOffset(now);

        Logger(Info, "The simulation will last %g seconds.\n", tend);
        Logger(Info, "Starting the main loop.\n");
        while (GetGlobalTime() <= tend) {
                
                // Wait for next period
		flag = clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &next, NULL);
                if (flag != 0) {
                        Logger(Critical, "Error in clock_nanosleep.\n");
                        break;
                }

                ProcessEvents();
                for (i=0; i<nEntities; i++)
                        entities[i]->readAndStoreInputs();
                IncreaseGlobalTime();
                for (i=0; i<nEntities; i++)
                        entities[i]->step();

                // Compute the time at which the thread will have to resume
	        next.tv_sec += interval.tv_sec;
	        next.tv_nsec += interval.tv_nsec;
	        tsnorm(&next);
        }
	flag = clock_gettime(CLOCK_REALTIME, &now);
        if (flag == 0) {
                Logger(Info, "Elapsed time: %g seconds.\n",
                        ((double) now.tv_sec + ((double) now.tv_nsec / NSEC_PER_SEC)) - GetGlobalTimeOffset());
        }

	// Switch to normal
	schedp.sched_priority = 0;
	flag = sched_setscheduler(0, SCHED_OTHER, &schedp);
        if (flag == 0)
                Logger(Info, "Switched to non real-time scheduler.\n");
}

#else

void NonRTSimulation(const std::vector<Entity*>& entities, double tend)
{
        size_t i, n = entities.size();
        double dt = GetGlobalDt();
        int nsteps = tend/dt;
        Logger(Debug, "tend = %e, dt = %e, nsteps = %d\n", tend, dt, nsteps);
        nsteps = 0;
        ResetGlobalTime();
        for (uint i=0; i<entities.size(); i++)
                entities[i]->initialise();
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
#if defined(HAVE_LIBLXRT)
        boost::thread thrd(RTSimulation, entities, tend);
#elif defined(HAVE_LIBRT)
        if (!CheckPrivileges())
                return;
        boost::thread thrd(RTSimulation, entities, tend);
#else
        boost::thread thrd(NonRTSimulation, entities, tend);
#endif
        thrd.join();
        Logger(Info, "Simulation thread has finished running.\n");
}

} // namespace dynclamp


