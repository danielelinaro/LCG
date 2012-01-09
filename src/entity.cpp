#include "events.h"
#include "thread_safe_queue.h"
#include "entity.h"

namespace dynclamp {

extern ThreadSafeQueue<Event*> eventsQueue;

Entity::Entity(uint id, double dt)
        : m_id(id), m_dt(dt), m_inputs(), m_pre(), m_post()
{}

Entity::~Entity()
{}

uint Entity::id() const
{
        return m_id;
}

void Entity::setDt(double dt)
{
        m_dt = dt;
}

double Entity::dt() const
{
        return m_dt;
}

void Entity::setParameters(const array& parameters)
{
        m_parameters = parameters;
}
        
void Entity::setParameter(double parameter, uint index)
{
        if (index >= m_parameters.size())
                throw "Parameter out of bounds.";
        m_parameters[index] = parameter;
}

const array& Entity::parameters() const
{
        return m_parameters;
}

double Entity::parameter(uint index) const
{
        if (index >= m_parameters.size())
                throw "Parameter out of bounds.";
        return m_parameters[index];
}

bool Entity::isPost(const Entity *entity) const
{
        for (int i=0; i<m_post.size(); i++) {
                if (entity->id() == m_post[i]->id())
                        return true;
        }
        return false;
}

void Entity::connect(Entity *entity)
{
        Logger(Debug, "--- Entity::connect(Entity*) ---\n");

        if (entity == this) {
                Logger(Debug, "Can't connect an entity to itself.\n");
                return;
        }

        if (isPost(entity)) {
                Logger(Debug, "Entity #%d was already connected to entity #%d.", id(), entity->id());
                return;
        }

        addPost(entity);
        entity->addPre(this, output());
}

const std::vector<Entity*> Entity::pre() const
{
        return m_pre;
}

const std::vector<Entity*> Entity::post() const
{
        return m_post;
}

void Entity::addPre(Entity *entity, double input)
{
        Logger(Debug, "--- Entity::addPre(Entity*, double) ---\n");
        m_pre.push_back(entity);
        m_inputs.push_back(input);
}

void Entity::addPost(Entity *entity)
{
        Logger(Debug, "--- Entity::addPost(Entity*, double) ---\n");
        m_post.push_back(entity);
}

void Entity::readAndStoreInputs()
{
        uint nInputs = m_pre.size();
        for (int i=0; i<nInputs; i++)
                m_inputs[i] = m_pre[i]->output();
        Logger(All, "Read all inputs to entity #%d.\n", id());
}

void Entity::handleEvent(const Event *event)
{}

void Entity::emitEvent(Event *event) const
{
#ifdef __APPLE__
        eventsQueue.push_back(event);
#else
        EnqueueEvent(event);
#endif
}

} // namespace dynclamp

