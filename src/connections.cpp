#include "connections.h"
#include "engine.h"
#include "common.h"
#include "synapses.h"

dynclamp::Entity* ConnectionFactory(string_dict& args)
{
        uint id;
        double delay;

        id = dynclamp::GetIdFromDictionary(args);
        if (! dynclamp::CheckAndExtractDouble(args, "delay", &delay)) {
                dynclamp::Logger(dynclamp::Critical, "Unable to build a Connection.\n");
                return NULL;
        }

        return new dynclamp::Connection(delay, id);
}

dynclamp::Entity* SynapticConnectionFactory(string_dict& args)
{
        uint id;
        double delay, weight;

        id = dynclamp::GetIdFromDictionary(args);
        if (! dynclamp::CheckAndExtractDouble(args, "delay", &delay) ||
            ! dynclamp::CheckAndExtractDouble(args, "weight", &weight)) {
                dynclamp::Logger(dynclamp::Critical, "Unable to build a SynapticConnection.\n");
                return NULL;
        }

        return new dynclamp::SynapticConnection(delay, weight, id);
}

dynclamp::Entity* VariableDelayConnectionFactory(string_dict& args)
{
        return new dynclamp::VariableDelayConnection(dynclamp::GetIdFromDictionary(args));
}

namespace dynclamp {

bool CompareFirst(const std::pair<double,Event*>& p1, 
                  const std::pair<double,Event*>& p2)
{
        return p1.first < p2.first;
}

Connection::Connection(double delay, uint id)
        : Entity(id), m_events()
{
        m_parameters["delay"] = delay;
        setName("Connection");
}
        
Connection::~Connection()
{
        clearEventsList();
}

void Connection::setDelay(double delay)
{
        if (delay >= 0)
                m_parameters["delay"] = delay;
        else
                Logger(Important, "Tried to set a negative delay.\n");
}

void Connection::step()
{
        std::list< std::pair<double,Event*> >::iterator it;
        for (it=m_events.begin(); it!=m_events.end(); it++)
                it->first -= GetGlobalDt();
        while (!m_events.empty() && m_events.front().first <= 0) {
                deliverEvent(m_events.front().second);
                delete m_events.front().second;
                m_events.pop_front();
        }
}

void Connection::deliverEvent(const Event *event)
{
        for (int i=0; i<m_post.size(); i++)
                m_post[i]->handleEvent(event);
}

double Connection::output()
{
        return 0.0;
}

bool Connection::initialise()
{
        clearEventsList();
        return true;
}

void Connection::terminate()
{
        clearEventsList();
}

void Connection::clearEventsList()
{
        if (!m_events.empty()) {
                std::list< std::pair<double,Event*> >::iterator it;
                for (it=m_events.begin(); it!=m_events.end(); it++)
                        delete it->second;
                m_events.clear();
        }
}

void Connection::handleEvent(const Event *event)
{
        m_events.push_back(std::make_pair(m_parameters["delay"] - GetGlobalDt(), new Event(*event)));
        m_events.sort(CompareFirst);
}

//~~~

SynapticConnection::SynapticConnection(double delay, double weight, uint id)
        : Connection(delay, id)
{
        m_parameters["weight"] = weight;
        setName("SynapticConnection");
}

void SynapticConnection::setWeight(double weight)
{
        m_parameters["weight"] = weight;
}

void SynapticConnection::deliverEvent(const Event *event)
{
        /*
        synapses::Synapse *syn;
        for (int i=0; i<m_post.size(); i++) {
                syn = dynamic_cast<synapses::Synapse*>(m_post[i]);
                syn->handleSpike(m_parameters["weight"]);
        }
        */
        emitEvent(new SpikeEvent(this, m_parameters["weight"]));
}

//~~~

VariableDelayConnection::VariableDelayConnection(uint id)
        : Connection(0, id)
{
        setName("VariableDelayConnection");
}

void VariableDelayConnection::handleEvent(const Event *event)
{
        double delay;
        do {
                delay = (*m_functor)();
        } while (delay == INFINITE || delay <= 0);
        setDelay(delay);
        Connection::handleEvent(event);
        Logger(Debug, "Connection #%d will deliver an event in %g seconds.\n", id(), delay);
}

void VariableDelayConnection::addPre(Entity *entity)
{
        Entity::addPre(entity);
        Functor *f = dynamic_cast<Functor*>(entity);
        if (f != NULL) {
                Logger(Debug, "A Functor (id #%d) was connected.\n", entity->id());
                m_functor = f;
        }
        else {
                Logger(Debug, "Entity #%d is not a functor.\n", entity->id());
        }
}

}//namespace dynclamp

