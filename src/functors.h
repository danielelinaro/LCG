#ifndef FUNCTOR_H
#define FUNCTOR_H

#include <time.h>
#include "entity.h"
#include "randlib.h"

namespace dynclamp {

/** 
* @class Functor
*
* @brief  Base abstract class for implementing functors.
*
* $Header $
*/
class Functor : public Entity
{
public:
        Functor(uint id = GetId());
        virtual double operator()() = 0;
        virtual void step();
        virtual double output();
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

/*!
 * @class RandomDelay
 * @brief A class that returns either uniformly or normally distributed random numbers.
 */
class RandomDelay : public Functor
{
public:
        /*! Creates a functor that returns uniform random numbers in the given interval. */
        RandomDelay(double interval[2], ullong seed = time(NULL), uint id = GetId());
        /*! Creates a functor that returns normal random numbers with given mean and standard deviation. */
        RandomDelay(double mean, double stddev, ullong seed = time(NULL), uint id = GetId());
        ~RandomDelay();
        virtual bool initialise();
        virtual double operator()();
private:
        /*!
         * Either a uniform or a normal RNG, depending on which constructor is used.
         */
        UniformRandom *rand;

        /*!
         * These two coefficients are used to compute a uniform and a normal
         * random number using the same expression: in fact, given a uniform
         * RNG in [0,1], a uniform random number on the interval [a,b] can be obtained
         * as n1 = a + (b-a)*uniform(0,1). In the same way, given a normal RNG with
         * zero mean and unitary standard deviation, a normal random number with mean
         * mu and standard deviation sigma can be obtained as n2 = mu + sigma*normal(0,1).
         * As a consequence, this class computes both uniform and random numbers using
         * the expression n = coeff[0] + coeff[1] * rand(), where rand is either a uniform
         * or a normal RNG, depending on which constructor is used to create an
         * instance of this class.
         */
        double m_coeffs[2];
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
dynclamp::Entity* RandomDelayFactory(string_dict& args);
        
#ifdef __cplusplus
}
#endif

#endif

