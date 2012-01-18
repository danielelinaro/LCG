#ifndef ENGINE_H
#define ENGINE_H

#include "types.h"
#include "utils.h"

namespace dynclamp {

class Entity;

void Simulate(const std::vector<Entity*>& entities, double tend);

} // namespace dynclamp

#endif // ENGINE_H

