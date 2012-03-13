#include "dynamical_entity.h"

namespace dynclamp {

DynamicalEntity::DynamicalEntity(uint id)
        : Entity(id)
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

