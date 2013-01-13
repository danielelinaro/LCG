/*=========================================================================
 *
 *   Program:     dynclamp
 *   Filename:    entity.cpp
 *
 *   Copyright (C) 2012 Daniele Linaro
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *   
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *   
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *=========================================================================*/

#include "entity.h"
#include "thread_safe_queue.h"

namespace dynclamp {

Entity::Entity(uint id)
        : m_id(id), m_inputs(), m_pre(), m_post(), m_name("Entity"), m_units("N/A")
{}

Entity::~Entity()
{
        terminate();
}

uint Entity::id() const
{
        return m_id;
}

size_t Entity::numberOfParameters() const
{
        return m_parameters.size();
}

const std::map<std::string,double>& Entity::parameters() const
{
        return m_parameters;
}

double& Entity::parameter(const std::string name)
{
        if (m_parameters.count(name) == 0)
                throw "No parameter with such a name";
        return m_parameters[name];
}

bool Entity::isPost(const Entity *entity) const
{
        Logger(All, "--- Entity::isPost(Entity*) ---\n");
        for (int i=0; i<m_post.size(); i++) {
                if (entity->id() == m_post[i]->id())
                        return true;
        }
        return false;
}

void Entity::connect(Entity *entity)
{
        Logger(All, "--- Entity::connect(Entity*) ---\n");

        if (entity == this) {
                Logger(Critical, "Can't connect an entity to itself (entity #%d).\n", entity->id());
                throw "Tried to connect entity to itself.";
        }

        if (isPost(entity)) {
                Logger(Info, "Entity #%d was already connected to entity #%d.\n", id(), entity->id());
                return;
        }

        addPost(entity);
        entity->addPre(this);
}

void Entity::terminate()
{}

const std::vector<Entity*>& Entity::pre() const
{
        return m_pre;
}

const std::vector<Entity*>& Entity::post() const
{
        return m_post;
}

void Entity::addPre(Entity *entity)
{
        Logger(All, "--- Entity::addPre(Entity*, double) ---\n");
        m_pre.push_back(entity);
        m_inputs.push_back(0);
        //m_inputs.reserve(m_inputs.size()+1);
}

void Entity::addPost(Entity *entity)
{
        Logger(All, "--- Entity::addPost(Entity*, double) ---\n");
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
        EnqueueEvent(event);
}

bool Entity::hasMetadata(size_t *ndims) const
{
        *ndims = 0;
        return false;
}

const double* Entity::metadata(size_t *dims, char *label) const
{
        return NULL;
}

const std::string& Entity::name() const
{
        return m_name;
}

const std::string& Entity::units() const
{
        return m_units;
}

void Entity::setName(const std::string& name)
{
        m_name = name;
}

void Entity::setUnits(const std::string& units)
{
        m_units = units;
}

} // namespace dynclamp

