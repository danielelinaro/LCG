#ifndef POISSON_GENERATOR_H
#define POISSON_GENERATOR_H

#include "types.h"
#include "utils.h"
#include "randlib.h"
#include "generator.h"
#include "generate_trial.h"

namespace dynclamp {

namespace generators {

class Poisson : public Generator {
public:
        Poisson(double rate, ullong seed = SEED, uint id = GetId(), double dt = GetGlobalDt());
        virtual bool hasNext() const;
        virtual double output() const;
        virtual void step();

private:
        void emitSpike(double timeout = 2e-3) const;
        void calculateTimeNextSpike();

private:
        double m_rate;
        UniformRandom m_random;
        double m_tNextSpike;
};

} // namespace generators

} // namespace dynclamp

#endif

