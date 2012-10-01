#ifndef CONNECTION_H
#define CONNECTION_H

#include "entity.h"
#include "functors.h"
#include <list>

namespace dynclamp {

class Connection : public Entity
{
public:
        Connection(double delay, double weight, uint id = GetId());
        ~Connection();
        
        void setDelay(double delay);
        void setWeight(double weight);

        virtual void step();
        virtual double output();
        virtual bool initialise();
        virtual void terminate();
        virtual void handleEvent(const Event *event);

protected:
        void clearEventsList();

protected:
        std::list< std::pair<double, Event*> > m_events;
};

class VariableDelayConnection : public Connection
{
public:
        VariableDelayConnection(double weight, uint id = GetId());
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

dynclamp::Entity* ConnectionFactory(string_dict& args);
dynclamp::Entity* VariableDelayConnectionFactory(string_dict& args);
        
#ifdef __cplusplus
}
#endif

#endif

