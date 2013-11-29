#include <pthread.h>
#include <stdio.h>
#include <assert.h>
#include <iostream>
#include "engine.h"
#include "entity.h"
#include "stream.h"
#include "utils.h"
#include "common.h"
#include "h5rec.h"

#ifdef HAVE_LIBLXRT
#include <rtai_lxrt.h>
#include <rtai_shm.h>

#define PRIORITY        0           /* 0 is the maximum */
#define STACK_SIZE      0           /* use default value */
#define MSG_SIZE        0           /* use default value */
#endif // HAVE_LIBLXRT

#ifdef HAVE_LIBANALOGY
#include <native/task.h>
#include <native/timer.h>
#endif // HAVE_LIBANALOGY

#ifdef HAVE_LIBRT
#include <errno.h>
#endif // HAVE_LIBRT

namespace lcg {

struct simulation_data {
        simulation_data(std::vector<Entity*>* entities, double tend)
                : m_entities(entities), m_tend(tend) {}
        std::vector<Entity*> *m_entities;
        double m_tend;
};

#ifdef REALTIME_ENGINE
double globalTimeOffset = 0.0;
#endif
#ifdef HAVE_LIBLXRT
double realtimeDt;
#endif

#if defined(HAVE_LIBLXRT)

void* RTSimulation(void *arg)
{
        simulation_data *data = static_cast<simulation_data*>(arg);
        std::vector<Entity*> *entities = data->m_entities;
        double tend = data->m_tend;
        RT_TASK *task;
        RTIME tickPeriod;
        RTIME currentTime, previousTime;
        RTIME start, stop;
        int preambleIterations, flag, i;
        unsigned long taskName;
        size_t nEntities = entities->size();
        int *retval = new int;
        *retval = -1;

        Logger(Debug, "\n>>>>> REAL TIME THREAD STARTED <<<<<\n\n");

#ifndef NO_STOP_RT_TIMER
        Logger(Debug, "Setting periodic mode.\n");
        rt_set_periodic_mode();
#endif

        Logger(Debug, "Starting the RT timer.\n");
        tickPeriod = start_rt_timer(sec2count(globalDt));
        
        Logger(Debug, "Initialising task.\n");
        taskName = nam2num("hybrid_simulator");
        task = rt_task_init(taskName, RT_SCHED_HIGHEST_PRIORITY+1, STACK_SIZE, MSG_SIZE);
        if(task == NULL) {
                Logger(Critical, "Error: cannot initialise real-time task.\n");
                pthread_exit((void *) retval);
        }

        Logger(Info, "The period is %.6g ms (f = %.6g Hz).\n", count2ms(tickPeriod), 1./count2sec(tickPeriod));

        Logger(Debug, "Switching to hard real time.\n");
        rt_make_hard_real_time();

        Logger(Debug, "Making the task periodic.\n");
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
        Logger(Debug, "Initialising all the entities.\n");
        for (i=0; i<nEntities; i++) {
                if (!entities->at(i)->initialise()) {
                        Logger(Critical, "Problems while initialising entity #%d. Aborting...\n", entities->at(i)->id());
                        pthread_exit((void *) retval);
                }
        }

        Logger(Important, "Expected duration: %g seconds.\n", tend);
        Logger(Debug, "Starting the main loop.\n");

        start = rt_timer_read();
        while (!TERMINATE_TRIAL() && GetGlobalTime() <= tend) {
                ProcessEvents();
                for (i=0; i<nEntities; i++)
                        entities->at(i)->readAndStoreInputs();
                IncreaseGlobalTime();
                for (i=0; i<nEntities; i++)
                        entities->at(i)->step();
                rt_task_wait_period();
        }
        stop = rt_timer_read();

        Logger(Important, "Elapsed time: %ld.%03ld ms\n",
                (long)(stop - start) / 1000000, (long)(stop - start) % 1000000);

        Logger(Debug, "Finished the main loop.\n");
        Logger(Debug, "Terminating all entities.\n");

        SetTrialRun(false);

        for (i=0; i<nEntities; i++)
                entities->at(i)->terminate();

stopRT:
        Logger(Debug, "Deleting the task.\n");
        rt_task_delete(task);

#ifndef NO_STOP_RT_TIMER
        Logger(Debug, "Stopping the RT timer.\n");
        stop_rt_timer();
#endif

        Logger(Debug, "\n>>>>> REAL TIME THREAD ENDED <<<<<\n\n");

        *retval = 0;
        pthread_exit((void *) retval);
}

#elif defined(HAVE_LIBANALOGY)

RT_TASK lcg_rt_task;

class TaskArgs {
public:
        TaskArgs(const std::vector<Entity*>& entities, double tend)
                : m_entities(entities), m_tend(tend) {}
        const std::vector<Entity*>& entities() const { return m_entities; }
        Entity* entity(int i) const { return m_entities[i]; }
        double tend() const { return m_tend; }
        int nEntities() const { return m_entities.size(); }
private:
        const std::vector<Entity*>& m_entities;
        double m_tend;
};

void RTSimulationTask(void *cookie)
{
        TaskArgs *arg = static_cast<TaskArgs*>(cookie);
        int flag, i;
        size_t nEntities = arg->nEntities();
        double tend = arg->tend();
        RTIME start, stop;

        SetGlobalTimeOffset();
        ResetGlobalTime();
        Logger(Debug, "Initialising all the entities.\n");
        for (i=0; i<nEntities; i++) {
                if (!arg->entity(i)->initialise()) {
                        Logger(Critical, "Problems while initialising entity #%d. Aborting...\n", arg->entity(i)->id());
                        return;
                }
        }

        Logger(Important, "Expected duration: %g seconds.\n", tend);
        Logger(Debug, "Starting the main loop.\n");

        flag = rt_task_set_periodic(NULL, TM_NOW, NSEC_PER_SEC*GetGlobalDt());
        if (flag != 0) {
                Logger(Critical, "Unable to make the task periodic.\n");
                return;
        }
        start = rt_timer_read();
        while (!TERMINATE_TRIAL() && GetGlobalTime() <= tend) {
                ProcessEvents();
                for (i=0; i<nEntities; i++)
                        arg->entity(i)->readAndStoreInputs();
                IncreaseGlobalTime();
                for (i=0; i<nEntities; i++)
                        arg->entity(i)->step();
                rt_task_wait_period(NULL);
        }
        stop = rt_timer_read();
        rt_task_set_periodic(NULL, TM_NOW, TM_INFINITE);

        Logger(Important, "Elapsed time: %ld.%03ld ms\n",
                (long)(stop - start) / 1000000, (long)(stop - start) % 1000000);

        Logger(Debug, "Finished the main loop.\n");
        Logger(Debug, "Terminating all entities.\n");

        SetTrialRun(false);

        for (i=0; i<nEntities; i++)
                arg->entity(i)->terminate();

        Logger(Debug, "Stopping the RT timer.\n");
}

void RTSimulation(const std::vector<Entity*>& entities, double tend, bool *retval)
{
        TaskArgs cookie(entities, tend);
        int flag;

        *retval = false;

        Logger(Debug, "\n>>>>> REAL TIME THREAD STARTED <<<<<\n\n");

        // Avoids memory swapping for this program
        mlockall(MCL_CURRENT | MCL_FUTURE);
        
        flag = rt_task_create(&lcg_rt_task, "lcg", 0, 99, T_CPU(3) | T_JOINABLE);
        // Create the task
        if (flag != 0) {
                Logger(Critical, "Unable to create the real-time task (err = %d).\n", flag);
                return;
        }
        Logger(Debug, "Successfully created the real-time task.\n");

        // Start the task
        flag = rt_task_start(&lcg_rt_task, RTSimulationTask, &cookie);
        if (flag != 0) {
                Logger(Critical, "Unable to start the real-time task (err = %d).\n", flag);
                rt_task_delete(&lcg_rt_task);
        }
        Logger(Debug, "Successfully started the real-time task.\n");

        // Wait for the task to finish
        flag = rt_task_join(&lcg_rt_task);
        if (flag == 0) {
                // There's no need to delete the task if we've joined successfully
                Logger(Debug, "Successfully joined with the child task.\n");
        }
        else {
                Logger(Important, "Unable to join with the child task.\n");
                // Delete the task
                Logger(Debug, "Deleting the task.\n");
                rt_task_delete(&lcg_rt_task);
        }

        Logger(Debug, "\n>>>>> REAL TIME THREAD ENDED <<<<<\n\n");

        *retval = true;
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

void* RTSimulation(void *arg)
{
        simulation_data *data = static_cast<simulation_data*>(arg);
        std::vector<Entity*> *entities = data->m_entities;
        double tend = data->m_tend;
        int priority, flag, i;
        size_t nEntities = entities->size();
        struct timespec now, period;
        struct sched_param schedp;
        int *retval = new int;
        *retval = -1;

        priority = sched_get_priority_max(SCHEDULER);
        if (priority < 0) {
                Logger(Critical, "Unable to get maximum priority.\n");
                perror("sched_get_priority_max");
                pthread_exit((void *) retval);
        }
        Logger(Debug, "The maximum priority is %d.\n", priority);

	memset(&schedp, 0, sizeof(schedp));
	schedp.sched_priority = priority;
	flag = sched_setscheduler(0, SCHEDULER, &schedp);
        if (flag != 0){
		Logger(Critical, "Unable to set maximum priority.\n");
                pthread_exit((void *) retval);
	}
        Logger(Debug, "Successfully set maximum priority.\n");

        // Reset simulation time
        ResetGlobalTime();

        // Initialise all entities
        for (i=0; i<nEntities; i++) {
                if (!entities->at(i)->initialise()) {
                        Logger(Critical, "Problems while initialising entity #%d. Aborting...\n", entities->at(i)->id());
                        pthread_exit((void *) retval);
                }
        }
        Logger(Debug, "Initialised all entities.\n");
        
        // The period of execution of the main loop
	period.tv_sec = 0;
	period.tv_nsec = GetGlobalDt() * NSEC_PER_SEC;

	// Get current time
	flag = clock_gettime(CLOCK_REALTIME, &now);
        if (flag < 0) {
                Logger(Critical, "Unable to get time from the system.\n");
                perror("clock_gettime");
                pthread_exit((void *) retval);
        }

        // Set the time offset, i.e. the absolute time of the beginning of the simulation
        SetGlobalTimeOffset(now);

        Logger(Important, "Expected duration: %g seconds.\n", tend);

        while (!TERMINATE_TRIAL() && GetGlobalTime() <= tend) {
                
                // Process the events and have all entities read their inputs
                ProcessEvents();
                for (i=0; i<nEntities; i++)
                        entities->at(i)->readAndStoreInputs();

                // Compute the time at which the thread will have to resume
	        now.tv_sec += period.tv_sec;
	        now.tv_nsec += period.tv_nsec;
	        tsnorm(&now);

                // Wait for next period
		flag = clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &now, NULL);
                if (flag != 0) {
                        Logger(Critical, "Error in clock_nanosleep.\n");
                        break;
                }

                // Increase the time of the simulation and step all entities forward
                IncreaseGlobalTime();
                for (i=0; i<nEntities; i++)
                        entities->at(i)->step();
        }

        // Compute how much time has passed since the beginning
        flag = clock_gettime(CLOCK_REALTIME, &now);
        if (flag == 0) {
                Logger(Important, "Elapsed time: %g seconds.\n",
                        ((double) now.tv_sec + ((double) now.tv_nsec / NSEC_PER_SEC)) - GetGlobalTimeOffset());
        }

        SetTrialRun(false);

        // Stop all entities
        Logger(Debug, "Terminating all entities.\n");
        for (i=0; i<nEntities; i++)
                entities->at(i)->terminate();
        Logger(Debug, "Terminated all entities.\n");

	// Switch to normal
	schedp.sched_priority = 0;
	flag = sched_setscheduler(0, SCHED_OTHER, &schedp);
        if (flag == 0)
                Logger(Debug, "Switched to non real-time scheduler.\n");

        *retval = 0;
        pthread_exit((void *) retval);
}

#else

void* NonRTSimulation(void *arg)
{        
        simulation_data *data = static_cast<simulation_data*>(arg);
        std::vector<Entity*> *entities = data->m_entities;
        double tend = data->m_tend;
	int i, nEntities = entities->size();
        double dt = GetGlobalDt();

        int *retval = new int;
        *retval = -1;

        ResetGlobalTime();

        for (i=0; i<nEntities; i++) {
                if (!entities->at(i)->initialise()) {
                        Logger(Critical, "Problems while initialising entity #%d. Aborting...\n", entities->at(i)->id());
                        pthread_exit((void *) retval);
                }
        }
        while (!TERMINATE_TRIAL() && GetGlobalTime() <= tend) {
                ProcessEvents();
                for (i=0; i<nEntities; i++)
                        entities->at(i)->readAndStoreInputs();
                IncreaseGlobalTime();
                for (i=0; i<nEntities; i++)
                        entities->at(i)->step();
        }

        SetTrialRun(false);

        for (i=0; i<nEntities; i++)
                entities->at(i)->terminate();

        *retval = 0;
        pthread_exit((void *) retval);
}

#endif // HAVE_LIBLXRT
 
int Simulate(std::vector<Entity*> *entities, double tend)
{
#ifdef HAVE_LIBRT
        if (!CheckPrivileges()) {
                return -1;
        }
#endif

        int *success, retval;
        pthread_t simulationThread;
        simulation_data data(entities, tend);

        SetTrialRun(true);
	ResetGlobalTime();

        H5RecorderCore *rec = NULL;
        for (int i=0; i<entities->size(); i++) {
                rec = dynamic_cast<H5RecorderCore*>(entities->at(i));
                if (rec) {
                        StartCommentsReaderThread(rec);
                        break;
                }
        }

#ifdef REALTIME_ENGINE
        pthread_create(&simulationThread, NULL, RTSimulation, (void *) &data);
#else
        pthread_create(&simulationThread, NULL, NonRTSimulation, (void *) &data);
#endif // REALTIME_ENGINE

        pthread_join(simulationThread, (void **) &success);
        retval = *success;
        delete success;

        StopCommentsReaderThread();

        if (retval == 0)
                Logger(Debug, "The simulation thread has terminated successfully.\n");
        else
                Logger(Important, "There were some problems with the simulation thread.\n");

        return retval;
}

int Simulate(std::vector<Stream*>* streams, double tend)
{
        int i, err;
        size_t len, ndims, *dims;
        char label[1024];
        ChunkedH5Recorder *rec = NULL;

        // initialise all the streams
        Logger(Debug, "Initializing all streams...\n");
        for (i=0; i<streams->size(); i++)
                streams->at(i)->initialise();

        // start the streams
        Logger(Debug, "Starting all streams...\n");
        for (i=0; i<streams->size(); i++)
                streams->at(i)->run(tend);

        // wait for each stream to finish, terminate it and then
        // save its data if there were no errors
        for (i=0; i<streams->size(); i++) {
                streams->at(i)->join(&err);
                streams->at(i)->terminate();
                if (!err) {
                        if (!rec)
                                rec = new ChunkedH5Recorder;
                        double_dict pars;
                        const double *data = streams->at(i)->data(&len);
                        rec->addRecord(streams->at(i)->id(), streams->at(i)->name().c_str(),
                                       streams->at(i)->units().c_str(), len, streams->at(i)->parameters());
                        rec->writeRecord(streams->at(i)->id(), data, len);
                        if (streams->at(i)->hasMetadata(&ndims)) {
                                dims = new size_t[ndims];
                                const double *metadata = streams->at(i)->metadata(dims, label);
                                if (ndims == 1)
                                        rec->writeMetadata(streams->at(i)->id(), metadata, 1, dims[0]);
                                else
                                        rec->writeMetadata(streams->at(i)->id(), metadata, dims[0], dims[1]);
                                delete dims;
                        }
                        Logger(Info, "Stream %d recorded for %.2f seconds.\n", streams->at(i)->id(), len*GetGlobalDt());
                }
                else {
                        Logger(Critical, "There were some problems during the recording in stream %d.\n",
                                        streams->at(i)->id());
                }
        }

        if (rec) {
                rec->writeRecordingDuration(len*GetGlobalDt());
                rec->writeTimeStep(GetGlobalDt());
                delete rec;
        }

        return 0;
}

} // namespace lcg

