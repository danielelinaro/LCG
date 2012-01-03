#include "dynamical_entity.h"

namespace dynclamp {

DynamicalEntity::DynamicalEntity(uint id, double dt)
        : Entity(id, dt)
{}

void DynamicalEntity::step()
{
        m_t += m_dt;
        evolve();
}

void DynamicalEntity::setParameters(const array& parameters)
{
        m_parameters = parameters;
}
        
void DynamicalEntity::setParameter(double parameter, uint index)
{
        if (index >= m_parameters.size())
                throw "Parameter out of bounds.";
        m_parameters[index] = parameter;
}

const array& DynamicalEntity::parameters() const
{
        return m_parameters;
}

double DynamicalEntity::parameter(uint index) const
{
        if (index >= m_parameters.size())
                throw "Parameter out of bounds.";
        return m_parameters[index];
}

} // namespace dynclamp

