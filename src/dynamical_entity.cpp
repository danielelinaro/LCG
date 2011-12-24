#include "dynamical_entity.h"


namespace dynclamp {

DynamicalEntity::DynamicalEntity(double dt)
{
        m_dt = dt;
}

DynamicalEntity::~DynamicalEntity()
{
}

void DynamicalEntity::setDt(double dt)
{
        m_dt = dt;
}

double DynamicalEntity::getDt() const
{
        return m_dt;
}

void DynamicalEntity::setInputEntity(const DynamicalEntity* entity)
{
        m_inputEntities.push_back(entity);
}

void DynamicalEntity::removeInputEntity(const DynamicalEntity* entity)
{
        m_inputEntities.remove(entity);
}

void DynamicalEntity::setInputs(const array& inputs)
{
        m_inputs = inputs;
}
        
void DynamicalEntity::setInput(double input, int index)
{
        if (index < 0 || index >= m_inputs.size())
                throw "Input out of bounds.";
        m_inputs[index] = input;
}

const array& DynamicalEntity::getInputs() const
{
        return m_inputs;
}

double DynamicalEntity::getInput(int index) const
{
        if (index < 0 || index >= m_inputs.size())
                throw "Input out of bounds.";
        return m_inputs[index];
}

void DynamicalEntity::setParameters(const array& parameters)
{
        m_parameters = parameters;
}
        
void DynamicalEntity::setParameter(double parameter, int index)
{
        if (index < 0 || index >= m_parameters.size())
                throw "Parameter out of bounds.";
        m_parameters[index] = parameter;
}

const array& DynamicalEntity::getParameters() const
{
        return m_parameters;
}

double DynamicalEntity::getParameter(int index) const
{
        if (index < 0 || index >= m_parameters.size())
                throw "Parameter out of bounds.";
        return m_parameters[index];
}

} // namespace dynclamp

