#ifndef STIMULUS_GENERATOR_H
#define STIMULUS_GENERATOR_H

#include "types.h"
#include "utils.h"
#include "generator.h"
#include "generate_trial.h"

namespace dynclamp {

namespace generators {

class Stimulus : public Generator {
public:
        Stimulus(const char *filename, uint id = GetId(), double dt = GetGlobalDt());
        virtual ~Stimulus();

        INT stimulusLength() const;
        virtual bool hasNext() const;

        virtual void step();
        virtual double output() const;

private:
        double m_srate;
        double *m_stimulus;
        INT m_stimulusLength;
        INT m_position;
};

} // namespace generators

} // namespace dynclamp

#endif

