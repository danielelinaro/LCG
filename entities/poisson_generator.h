#ifndef POISSON_GENERATOR_H
#define POISSON_GENERATOR_H

#include "types.h"
#include "utils.h"
#include "randlib.h"
#include "generator.h"

#define POISSON_RATE m_parameters["rate"]

namespace lcg {

namespace generators {

class Poisson : public Generator {
public:
        Poisson(double rate, ullong seed, uint id = GetId());
        virtual bool initialise();
        virtual bool hasNext() const;
        virtual double output();
        virtual void step();

private:
        void calculateTimeNextSpike();

private:
        UniformRandom m_random;
        double m_tNextSpike;
};

} // namespace generators

} // namespace lcg

/***
 *   FACTORY METHODS
 ***/
#ifdef __cplusplus
extern "C" {
#endif

lcg::Entity* PoissonFactory(string_dict& args);
	
#ifdef __cplusplus
}
#endif


#endif

