#ifndef GENERATOR_H
#define GENERATOR_H

#include "entity.h"

namespace dynclamp {

namespace generators {

class Generator : public Entity {
public:
        Generator(uint id = GetId()) : Entity(id) {}
        virtual ~Generator() {}

        virtual bool hasNext() const = 0;
};

} // namespace generators

} // namespace dynclamp

#endif

