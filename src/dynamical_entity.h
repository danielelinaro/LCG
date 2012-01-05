#ifndef DYNAMICAL_ENTITY_H
#define DYNAMICAL_ENTITY_H

#include "entity.h"
#include "types.h"
#include "utils.h"

namespace dynclamp
{

class DynamicalEntity;

class DynamicalEntity : public Entity
{
public:
        DynamicalEntity(uint id = GetId(), double dt = GetGlobalDt());

        const array& state() const;

        virtual void step();

protected:
        virtual void evolve() = 0;

protected:
        array  m_state;
};

}

#endif

