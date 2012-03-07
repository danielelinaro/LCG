#ifndef ENGINE_H
#define ENGINE_H

#include "types.h"
#include "utils.h"

namespace dynclamp {

class Entity;

void Simulate(const std::vector<Entity*>& entities, double tend);

void ResetGlobalTime();
void IncreaseGlobalTime();
void IncreaseGlobalTime(double dt);
double GetGlobalTime();
double SetGlobalDt(double dt);
double GetGlobalDt();
#ifdef HAVE_LIBLXRT
void SetGlobalTimeOffset();
double GetGlobalTimeOffset();
#endif // HAVE_LIBLXRT

} // namespace dynclamp

#endif // ENGINE_H

