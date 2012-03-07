#ifndef ENTITY_H
#define ENTITY_H

#include "types.h"
#include "utils.h"
#include "events.h"
#include "engine.h"

namespace dynclamp
{

class Entity;

class Entity
{
public:
        Entity(uint id = GetId(), double dt = GetGlobalDt());
        virtual ~Entity();

        uint id() const;

        void setDt(double dt);
        double dt() const;

        void setParameters(const array& parameters);
        void setParameter(double parameter, uint index);
        size_t numberOfParameters() const;
        const array& parameters() const;
        double parameter(uint index) const;

        // connect this entity TO the one passed as a parameter,
        // i.e., this entity will be an input of the one passed
        // as a parameter.
        void connect(Entity* entity);

        const std::vector<Entity*>& pre() const;
        const std::vector<Entity*>& post() const;

        void readAndStoreInputs();

        virtual void step() = 0;

        virtual double output() const = 0;

        virtual void handleEvent(const Event *event);

        virtual void emitEvent(Event *event) const;

        virtual bool hasMetadata(size_t *ndims) const;
        virtual const double* metadata(size_t *dims, char *label) const;

protected:
        virtual void addPre(Entity *entity);
        virtual void addPost(Entity *entity);

private:
        bool isPost(const Entity *entity) const;

protected:
        uint   m_id;
        double m_dt;

        array  m_parameters;
        array  m_inputs;

        std::vector<Entity*> m_pre;
        std::vector<Entity*> m_post;
};

}

#endif

