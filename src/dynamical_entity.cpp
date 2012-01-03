#include "dynamical_entity.h"
#include <map>

namespace dynclamp {

std::map< uint, std::map< uint, const DynamicalEntity* > > connectionsMatrix;

DynamicalEntity::DynamicalEntity(uint id, double dt)
{
        m_id = id;
        m_t = 0;
        m_dt = dt;
}

DynamicalEntity::~DynamicalEntity()
{}

uint DynamicalEntity::id() const
{
        return m_id;
}

void DynamicalEntity::setDt(double dt)
{
        m_dt = dt;
}

double DynamicalEntity::dt() const
{
        return m_dt;
}

void DynamicalEntity::step()
{
        m_t += m_dt;
        evolve();
}

bool DynamicalEntity::isPost(const DynamicalEntity *entity) const
{
        for (int i=0; i<m_post.size(); i++) {
                if (entity->id() == m_post[i]->id())
                        return true;
        }
        return false;
}

void DynamicalEntity::connect(DynamicalEntity *entity)
{
        Logger(Debug, "DynamicalEntity::connect(DynamicalEntity *)\n");

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

const std::vector<DynamicalEntity*> DynamicalEntity::pre() const
{
        return m_pre;
}

const std::vector<DynamicalEntity*> DynamicalEntity::post() const
{
        return m_post;
}

void DynamicalEntity::addPre(DynamicalEntity *entity, double input)
{
        m_pre.push_back(entity);
        m_inputs.push_back(input);
}

void DynamicalEntity::addPost(DynamicalEntity *entity)
{
        m_post.push_back(entity);
}

void DynamicalEntity::readAndStoreInputs()
{
        uint nInputs = m_pre.size();
        for (int i=0; i<nInputs; i++)
                m_inputs[i] = m_pre[i]->output();
        Logger(Debug, "Read all inputs to entity #%d.\n", id());
}

void DynamicalEntity::setParameters(const array& parameters)
{
        m_parameters = parameters;
}
        
void DynamicalEntity::setParameter(double parameter, uint index)
{
        if (index >= m_parameters.size())
                throw "Parameter out of bounds.";
        m_parameters[index] = parameter;
}

const array& DynamicalEntity::parameters() const
{
        return m_parameters;
}

double DynamicalEntity::parameter(uint index) const
{
        if (index >= m_parameters.size())
                throw "Parameter out of bounds.";
        return m_parameters[index];
}

void DynamicalEntity::handleEvent(const Event *event)
{}

} // namespace dynclamp

