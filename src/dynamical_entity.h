#ifndef DYNAMICAL_ENTITY_H
#define DYNAMICAL_ENTITY_H

#include "types.h"
#include "utils.h"
#include "events.h"
#include <list>

namespace dynclamp
{

class DynamicalEntity;

class DynamicalEntity
{
public:
        DynamicalEntity(uint id = GetId(), double dt = GetGlobalDt());
        virtual ~DynamicalEntity();

        uint getId() const;

        void setDt(double dt);
        double getDt() const;

        void setInputEntity(const DynamicalEntity* entity);
        void removeInputEntity(const DynamicalEntity* entity);

        void setInputs(const array& inputs);
        void setInput(double input, uint index);
        const array& getInputs() const;
        double getInput(uint index) const;
        
        void setParameters(const array& parameters);
        void setParameter(double parameter, uint index);
        const array& getParameters() const;
        double getParameter(uint index) const;

        virtual double getOutput() const = 0;
        virtual void step() = 0;
        virtual void handleEvent(const Event *event);

protected:
        uint   m_id;
        double m_dt;

        std::list<const DynamicalEntity*> m_inputEntities;
        array  m_state;
        array  m_parameters;
        array  m_inputs;

};

}

#endif

