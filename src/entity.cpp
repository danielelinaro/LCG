#include "entity.h"
#include <map>

namespace dynclamp {

std::map< uint, std::map< uint, const Entity* > > connectionsMatrix;

Entity::Entity(uint id, double dt)
{
        m_id = id;
        m_t = 0;
        m_dt = dt;
}

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
        Logger(Debug, "Entity::connect(Entity *)\n");

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
        m_pre.push_back(entity);
        m_inputs.push_back(input);
}

void Entity::addPost(Entity *entity)
{
        m_post.push_back(entity);
}

void Entity::readAndStoreInputs()
{
        uint nInputs = m_pre.size();
        for (int i=0; i<nInputs; i++)
                m_inputs[i] = m_pre[i]->output();
        Logger(Debug, "Read all inputs to entity #%d.\n", id());
}

void Entity::handleEvent(const Event *event)
{}

} // namespace dynclamp

