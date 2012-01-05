#ifndef GENERATOR_H
#define GENERATOR_H

#include "entity.h"

namespace dynclamp {

namespace generators {

class Generator : public Entity {
public:
        Generator(uint id = GetId(), double dt = GetGlobalDt())
                : Entity(id, dt) {}
        virtual ~Generator() {}

        virtual bool hasNext() const = 0;
};

} // namespace generators

} // namespace dynclamp

#endif

