#include "connections.h"
#include "engine.h"

dynclamp::Entity* ConnectionFactory(dictionary& args)
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

dynclamp::Entity* VariableDelayConnectionFactory(dictionary& args)
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
        : Entity(id), m_delay(delay), m_events()
{
        setName("Connection");
}
        
Connection::~Connection()
{
        clearEventsList();
}

double Connection::delay() const
{
        return m_delay;
}

void Connection::setDelay(double delay)
{
        if (delay >= 0)
                m_delay = delay;
        else
                Logger(Important, "Tried to set a negative delay.\n");
}

void Connection::step()
{
        std::list< std::pair<double,Event*> >::iterator it;
        for (it=m_events.begin(); it!=m_events.end(); it++)
                it->first -= GetGlobalDt();
        while (m_events.front().first <= 0) {
                emitEvent(new Event(m_events.front().second->type(), this));
                delete m_events.front().second;
                m_events.pop_front();
        }
}

double Connection::output() const
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
        m_events.push_back(std::make_pair(m_delay-GetGlobalDt(), new Event(*event)));
        m_events.sort(CompareFirst);
}

VariableDelayConnection::VariableDelayConnection(uint id)
        : Connection(0, id)
{
        setName("VariableDelayConnection");
}

void VariableDelayConnection::handleEvent(const Event *event)
{
        double delay = (*m_functor)();
        if (delay != std::numeric_limits<double>::infinity()) {
                // we don't handle events with an infinite delay
                setDelay(delay);
                Connection::handleEvent(event);
        }
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

}

