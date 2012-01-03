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
{
}

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

/*
void DynamicalEntity::connect(DynamicalEntity* entity)
{
        Logger(Debug, "DynamicalEntity::connect(DynamicalEntity *)\n");

        if (entity == this) {
                Logger(Debug, "Can't connect an entity to itself.\n");
                return;
        }

        uint idPre = id();
        uint idPost = entity->id();

        if (connectionsMatrix.count(idPost) == 0) {
                connectionsMatrix[idPost] = std::map<uint,const DynamicalEntity*>();
                Logger(Debug, "Entity #%d had no inputs.\n", idPost);
        }

        if (connectionsMatrix[idPost].count(idPre) == 0) {
                connectionsMatrix[idPost][idPre] = this;
                entity->m_inputs.push_back(0.0);
                Logger(Debug, "Connecting entity #%d to entity #%d.\n", idPre, idPost);
        }
        else {
                Logger(Debug, "Entity #%d was already connected to entity #%d.", idPre, idPost);
        }

        finalizeConnect(entity);
}

void DynamicalEntity::finalizeConnect(DynamicalEntity *entity)
{}

void DynamicalEntity::readAndStoreInputs()
{
        Logger(Debug, "DynamicalEntity::readAndStoreInputs()\n");
        uint i;
        uint myId = id();
        uint nInputs = connectionsMatrix[myId].size();
        Logger(Debug, "Entity #%d has %d inputs.\n", myId, nInputs);
        std::map< uint, const DynamicalEntity*>::iterator it;
        for (it = connectionsMatrix[myId].begin(), i = 0; it != connectionsMatrix[myId].end(); it++, i++) {
                m_inputs[i] = it->second->output();
        }
        Logger(Debug, "Read all inputs to entity #%d.\n", myId);
}
*/

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

        m_post.push_back(entity);
        entity->m_pre.push_back(this);
        entity->m_inputs.push_back(0.0);
}

const std::vector<DynamicalEntity*> DynamicalEntity::pre() const
{
        return m_pre;
}

const std::vector<DynamicalEntity*> DynamicalEntity::post() const
{
        return m_post;
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
{
}

} // namespace dynclamp

