#include "dynamical_entity.h"

namespace lcg {

DynamicalEntity::DynamicalEntity(uint id)
        : Entity(id)
{
        setName("DynamicalEntity");
}

const array& DynamicalEntity::state() const
{
        return m_state;
}

void DynamicalEntity::step()
{
        evolve();
}

} // namespace lcg

