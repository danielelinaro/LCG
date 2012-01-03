#ifndef DYNAMICAL_ENTITY_H
#define DYNAMICAL_ENTITY_H

#include <vector>
#include "types.h"
#include "utils.h"
#include "events.h"

namespace dynclamp
{

class DynamicalEntity;

class DynamicalEntity
{
public:
        DynamicalEntity(uint id = GetId(), double dt = GetGlobalDt());
        virtual ~DynamicalEntity();

        uint id() const;

        void setDt(double dt);
        double dt() const;

        // connect this entity TO the one passed as a parameter,
        // i.e., this entity will be an input of the one passed
        // as a parameter.
        void connect(DynamicalEntity* entity);

        const std::vector<DynamicalEntity*> pre() const;
        const std::vector<DynamicalEntity*> post() const;

        void setParameters(const array& parameters);
        void setParameter(double parameter, uint index);
        const array& parameters() const;
        double parameter(uint index) const;

        void readAndStoreInputs();
        void step();
        virtual double output() const = 0;

        virtual void handleEvent(const Event *event);

protected:
        virtual void evolve() = 0;

private:
        bool isPost(const DynamicalEntity *entity) const;
        void addPre(DynamicalEntity *entity, double input);
        void addPost(DynamicalEntity *entity);

protected:
        uint   m_id;
        double m_t;
        double m_dt;

        array  m_state;
        array  m_parameters;
        array  m_inputs;

        std::vector<DynamicalEntity*> m_pre;
        std::vector<DynamicalEntity*> m_post;

};

}

#endif

