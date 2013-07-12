#ifndef ENGINE_H
#define ENGINE_H

#include "types.h"
#include "utils.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

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

namespace lcg {

int Simulate(std::vector<Entity*> *entities, double tend);

#ifdef REALTIME_ENGINE
extern double globalTimeOffset;
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

} // namespace lcg

#endif // ENGINE_H

