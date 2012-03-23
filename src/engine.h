#ifndef ENGINE_H
#define ENGINE_H

#include "types.h"
#include "utils.h"

namespace dynclamp {

double SetGlobalDt(double dt);
void Simulate(const std::vector<Entity*>& entities, double tend);

extern double globalT;
extern double globalDt;

#define GetGlobalDt() globalDt
#define GetGlobalTime() globalT
#define IncreaseGlobalTime() (globalT += globalDt)
#define ResetGlobalTime()  (globalT = 0.0)

#ifdef HAVE_LIBLXRT
#define NO_STOP_RT_TIMER
extern double realtimeDt;
extern double globalTimeOffset;
#undef GetGlobalDt
#define GetGlobalDt() realtimeDt
#undef IncreaseGlobalTime
#define IncreaseGlobalTime() (globalT += realtimeDt)
#define SetGlobalTimeOffset() (globalTimeOffset = count2sec(rt_get_time()))
#define GetGlobalTimeOffset() globalTimeOffset
#endif // HAVE_LIBLXRT

} // namespace dynclamp

#endif // ENGINE_H

