#ifndef FUNCTOR_H
#define FUNCTOR_H

#include "entity.h"

namespace dynclamp {

class Functor : public Entity
{
public:
        Functor(uint id = GetId());
        virtual double operator()() = 0;
        virtual void step();
        virtual double output() const;
        virtual bool initialise();
};

class SobolDelay : public Functor
{
public:
        SobolDelay(uint id = GetId());
        virtual bool initialise();
        virtual double operator()();
private:
        int m_numberOfSobolSequences;
};

}

/***
 *   FACTORY METHODS
 ***/
#ifdef __cplusplus
extern "C" {
#endif

dynclamp::Entity* SobolDelayFactory(dictionary& args);
        
#ifdef __cplusplus
}
#endif

#endif

