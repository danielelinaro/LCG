#ifndef ENGINE_H
#define ENGINE_H

#include "types.h"
#include "utils.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

namespace dynclamp {

double SetGlobalDt(double dt);
void Simulate(const std::vector<Entity*>& entities, double tend);

extern double globalT;
extern double globalDt;

#define GetGlobalDt() globalDt
#define GetGlobalTime() globalT
#define IncreaseGlobalTime() (globalT += globalDt)
#define ResetGlobalTime()  (globalT = 0.0)

#ifdef REALTIME_ENGINE
extern double globalTimeOffset;
#define NSEC_PER_SEC 1000000000
#define GetGlobalTimeOffset() globalTimeOffset
#endif

#ifdef HAVE_LIBLXRT
extern double realtimeDt;
//#define NO_STOP_RT_TIMER
#undef GetGlobalDt
#define GetGlobalDt() realtimeDt
#undef IncreaseGlobalTime
#define IncreaseGlobalTime() (globalT += realtimeDt)
#define SetGlobalTimeOffset() (globalTimeOffset = count2sec(rt_get_time()))
#endif // HAVE_LIBLXRT

#ifdef HAVE_LIBANALOGY
#include <sys/mman.h>
#define SetGlobalTimeOffset() (globalTimeOffset = ((double) rt_timer_ticks2ns(rt_timer_read())) / NSEC_PER_SEC)
#endif // HAVE_LIBANALOGY

#ifdef HAVE_LIBRT
#define SCHEDULER SCHED_RR
#define SetGlobalTimeOffset(now) (globalTimeOffset = now.tv_sec + ((double) now.tv_nsec / NSEC_PER_SEC))
#endif // HAVE_LIBRT

} // namespace dynclamp

#endif // ENGINE_H

