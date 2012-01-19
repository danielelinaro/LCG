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

        uint stimulusLength() const;
        virtual bool hasNext() const;

        virtual void step();
        virtual double output() const;

        virtual bool hasMetadata(size_t *ndims) const;
        virtual const double* metadata(size_t *dims, char *label) const;

        double duration() const;

private:
        double *m_stimulus;
        uint m_stimulusLength;
        uint m_position;

        double *m_stimulusMetadata;
        size_t m_stimulusRows, m_stimulusCols;
};

} // namespace generators

} // namespace dynclamp

/***
 *   FACTORY METHODS
 ***/
#ifdef __cplusplus
extern "C" {
#endif

dynclamp::Entity* StimulusFactory(dictionary& args);
	
#ifdef __cplusplus
}
#endif

#endif

