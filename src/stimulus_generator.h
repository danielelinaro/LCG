#ifndef STIMULUS_GENERATOR_H
#define STIMULUS_GENERATOR_H

#include "types.h"
#include "utils.h"
#include "entity.h"
#include "generate_trial.h"

namespace dynclamp {

class StimulusGenerator : public Entity {
public:
        StimulusGenerator(const char *filename, uint id = GetId(), double dt = GetGlobalDt());
        ~StimulusGenerator();

        INT stimulusLength() const;
        bool hasNext() const;

        virtual double output();

private:
        double m_dt;
        double m_srate;
        double *m_stimulus;
        INT m_stimulusLength;
        INT m_position;
};

} // namespace dynclamp

#endif

