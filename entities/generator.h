#ifndef GENERATOR_H
#define GENERATOR_H

#include "entity.h"

namespace lcg {

namespace generators {

class Generator : public Entity {
public:
        Generator(uint id = GetId()) : Entity(id) {
                setName("Generator");
        }
        virtual ~Generator() {}

        virtual bool hasNext() const = 0;
};

} // namespace generators

} // namespace lcg

#endif

