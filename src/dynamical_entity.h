#ifndef DYNAMICAL_ENTITY_H
#define DYNAMICAL_ENTITY_H

#include "types.h"
#include <list>

namespace dynclamp
{

class DynamicalEntity;

class DynamicalEntity
{
public:
        DynamicalEntity(double dt);
        virtual ~DynamicalEntity();

        void setDt(double dt);
        double getDt() const;

        void setInputEntity(const DynamicalEntity* entity);
        void removeInputEntity(const DynamicalEntity* entity);

        void setInputs(const array& inputs);
        void setInput(double input, int index);
        const array& getInputs() const;
        double getInput(int index) const;

        virtual double getOutput() const = 0;
        
        void setParameters(const array& parameters);
        void setParameter(double parameter, int index);
        const array& getParameters() const;
        double getParameter(int index) const;

        virtual void step() = 0;

protected:
        std::list<const DynamicalEntity*> m_inputEntities;
        array  m_state;
        array  m_parameters;
        array  m_inputs;

        double m_dt;
};

}

#endif

