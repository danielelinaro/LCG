#ifndef ENTITY_H
#define ENTITY_H

#include <vector>
#include "types.h"
#include "utils.h"
#include "events.h"

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

        // connect this entity TO the one passed as a parameter,
        // i.e., this entity will be an input of the one passed
        // as a parameter.
        void connect(Entity* entity);

        const std::vector<Entity*> pre() const;
        const std::vector<Entity*> post() const;

        void readAndStoreInputs();
        virtual double output() = 0;

        virtual void handleEvent(const Event *event);

private:
        bool isPost(const Entity *entity) const;
        void addPre(Entity *entity, double input);
        void addPost(Entity *entity);

protected:
        uint   m_id;
        double m_t;
        double m_dt;

        array  m_inputs;

        std::vector<Entity*> m_pre;
        std::vector<Entity*> m_post;
};

}

#endif

