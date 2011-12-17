#ifndef DYNAMICAL_ENTITY_H
#define DYNAMICAL_ENTITY_H

#include <list>
#include <vector>

namespace dynclamp
{

class DynamicalEntity;

class DynamicalEntity
{
private:
        std::vector<DynamicalEntity*> m_inputEntities;
        std::vector<double> m_state;
        std::vector<double> m_parameters;
        std::vector<double> m_inputs;
        double m_output;
};

}

#endif

