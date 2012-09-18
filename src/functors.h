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
        virtual double output();
        virtual bool initialise();
};
/** 
* @class Functor
*
* @brief  Base abstract class for implementing functors.
*
* $Header $
*/

class SobolDelay : public Functor
{
public:
        SobolDelay(uint id = GetId());
        virtual bool initialise();
        virtual double operator()();
private:
        int m_numberOfSobolSequences;
};
/**
* @class PhasicDelay
*
* @brief Is a functor that returns input*(fixed delay).
* PhasicDelay(id,phase) has an operator which returns the input times a phase.
* "delay" can be specified in the configuration file. Any value can be used, 
* suggestion is between 0 and 1.
* 
* $Header$
*/
class PhasicDelay : public Functor
{
public:
        PhasicDelay(double phase = 0.0, uint id = GetId());
        virtual bool initialise();
        virtual double operator()();
};

} //namespace dynclamp

/***
 *   FACTORY METHODS
 ***/
#ifdef __cplusplus
extern "C" {
#endif

dynclamp::Entity* SobolDelayFactory(string_dict& args);
dynclamp::Entity* PhasicDelayFactory(string_dict& args);
        
#ifdef __cplusplus
}
#endif

#endif

