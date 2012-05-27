#ifndef ENTITY_H
#define ENTITY_H

#include <vector>
#include <string>

#include "types.h"
#include "utils.h"
#include "events.h"

namespace dynclamp
{

class Entity;

class Entity
{
public:
        Entity(uint id = GetId());
        virtual ~Entity();

        uint id() const;

        void setParameters(const array& parameters);
        void setParameter(double parameter, uint index);
        size_t numberOfParameters() const;
        const array& parameters() const;
        double parameter(uint index) const;

        /**
         * Connects this entity to the one passed as a parameter,
         * i.e., this entity will be an input of the one passed
         * as a parameter.
         */
        void connect(Entity* entity);

        const std::vector<Entity*>& pre() const;
        const std::vector<Entity*>& post() const;

        void readAndStoreInputs();

        const std::string& name() const;
        const std::string& units() const;

        virtual void step() = 0;

        virtual double output() const = 0;

        virtual bool initialise() = 0;

        virtual void terminate();

        virtual void handleEvent(const Event *event);

        virtual void emitEvent(Event *event) const;

        virtual bool hasMetadata(size_t *ndims) const;
        virtual const double* metadata(size_t *dims, char *label) const;

protected:
        virtual void addPre(Entity *entity);
        virtual void addPost(Entity *entity);
        void setName(const std::string& name);
        void setUnits(const std::string &units);

private:
        bool isPost(const Entity *entity) const;

protected:
        uint   m_id;

        array  m_parameters;
        array  m_inputs;

        std::vector<Entity*> m_pre;
        std::vector<Entity*> m_post;

private:
        std::string m_name, m_units;
};

}

#endif

