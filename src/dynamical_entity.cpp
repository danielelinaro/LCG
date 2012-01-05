#include "dynamical_entity.h"

namespace dynclamp {

DynamicalEntity::DynamicalEntity(uint id, double dt)
        : Entity(id, dt)
{}

const array& DynamicalEntity::state() const
{
        return m_state;
}

void DynamicalEntity::step()
{
        evolve();
}

} // namespace dynclamp

