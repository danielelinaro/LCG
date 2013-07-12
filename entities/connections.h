#ifndef CONNECTION_H
#define CONNECTION_H

#include "entity.h"
#include "functors.h"
#include <list>

namespace lcg {

class Connection : public Entity
{
public:
        Connection(double delay, uint id = GetId());
        ~Connection();
        
        void setDelay(double delay);

        virtual void step();
        virtual double output();
        virtual bool initialise();
        virtual void terminate();
        virtual void handleEvent(const Event *event);

protected:
        virtual void deliverEvent(const Event *event);
        void clearEventsList();

protected:
        std::list< std::pair<double, Event*> > m_events;
};

class SynapticConnection : public Connection
{
public:
        SynapticConnection(double delay, double weight, uint id = GetId());
        void setWeight(double weight);
protected:
        virtual void deliverEvent(const Event *event);
};

class VariableDelayConnection : public Connection
{
public:
        VariableDelayConnection(uint id = GetId());
        virtual void handleEvent(const Event *event);
protected:
        virtual void addPre(Entity *entity);
private:
        Functor *m_functor;
};

}

/***
 *   FACTORY METHODS
 ***/
#ifdef __cplusplus
extern "C" {
#endif

lcg::Entity* ConnectionFactory(string_dict& args);
lcg::Entity* SynapticConnectionFactory(string_dict& args);
lcg::Entity* VariableDelayConnectionFactory(string_dict& args);
        
#ifdef __cplusplus
}
#endif

#endif

