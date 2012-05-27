#ifndef STIMULUS_GENERATOR_H
#define STIMULUS_GENERATOR_H

#include "entity.h"
#include "types.h"
#include "utils.h"
#include "generator.h"

namespace dynclamp {

namespace generators {

class Waveform : public Generator {
public:
        Waveform(const char *filename = NULL, bool triggered = false, uint id = GetId());
        virtual ~Waveform();

        bool setFilename(const char *filename);
        virtual bool initialise();

        uint stimulusLength() const;
        virtual bool hasNext() const;

        virtual void step();
        virtual void terminate();

        virtual double output() const; 
        virtual void handleEvent(const Event *event);
        virtual bool hasMetadata(size_t *ndims) const;
        virtual const double* metadata(size_t *dims, char *label) const;

        double duration() const;

private:
        void freeMemory();

protected:
        virtual void reset();

protected:
        char m_filename[FILENAME_MAXLEN];

        double *m_stimulus;
        uint m_stimulusLength;
        uint m_position;
        bool m_triggered;
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

dynclamp::Entity* WaveformFactory(dictionary& args);
	
#ifdef __cplusplus
}
#endif

#endif

