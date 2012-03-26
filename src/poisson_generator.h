#ifndef POISSON_GENERATOR_H
#define POISSON_GENERATOR_H

#include "types.h"
#include "utils.h"
#include "randlib.h"
#include "generator.h"

#define POISSON_RATE m_parameters[0]

namespace dynclamp {

namespace generators {

class Poisson : public Generator {
public:
        Poisson(double rate, ullong seed, uint id = GetId());
        virtual void initialise();
        virtual bool hasNext() const;
        virtual double output() const;
        virtual void step();

private:
        void emitSpike(double timeout = 2e-3) const;
        void calculateTimeNextSpike();

private:
        UniformRandom m_random;
        double m_tNextSpike;
};

} // namespace generators

} // namespace dynclamp

/***
 *   FACTORY METHODS
 ***/
#ifdef __cplusplus
extern "C" {
#endif

dynclamp::Entity* PoissonFactory(dictionary& args);
	
#ifdef __cplusplus
}
#endif


#endif

