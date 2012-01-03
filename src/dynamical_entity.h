#ifndef DYNAMICAL_ENTITY_H
#define DYNAMICAL_ENTITY_H

#include <vector>
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

        void setParameters(const array& parameters);
        void setParameter(double parameter, uint index);
        const array& parameters() const;
        double parameter(uint index) const;

        void step();

protected:
        virtual void evolve() = 0;

protected:
        array  m_state;
        array  m_parameters;

};

}

#endif

