#ifndef CONNECTION_H
#define CONNECTION_H

#include "entity.h"
#include "functors.h"
#include <list>
#include <utility>

namespace dynclamp {

class Connection : public Entity
{
public:
        Connection(double delay, uint id = GetId());
        ~Connection();
        
        double delay() const;
        void setDelay(double delay);

        virtual void step();
        virtual double output() const;
        virtual bool initialise();
        virtual void terminate();
        virtual void handleEvent(const Event *event);

protected:
        void clearEventsList();

protected:
        double m_delay;
        std::list< std::pair<double, Event*> > m_events;
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

dynclamp::Entity* ConnectionFactory(dictionary& args);
dynclamp::Entity* VariableDelayConnectionFactory(dictionary& args);
        
#ifdef __cplusplus
}
#endif

#endif

