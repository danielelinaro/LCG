#ifndef WAVEFORM_H
#define WAVEFORM_H

#include "entity.h"
#include "types.h"
#include "utils.h"
#include "generator.h"

namespace dynclamp {

namespace generators {

class Waveform : public Generator {
public:
        Waveform(const char *stimulusFile = NULL, bool triggered = false,
                 const std::string& units = "N/A", uint id = GetId());
        virtual ~Waveform();

        bool setStimulusFile(const char *filename);
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
        bool parseStimulusFile();
        void freeMemory();

protected:
        virtual void reset();

protected:
        char m_stimulusFile[FILENAME_MAXLEN];

        double *m_stimulus;
        uint m_stimulusLength;
        uint m_position;
        bool m_triggered;
        double *m_stimulusMetadata;
        size_t m_stimulusRows, m_stimulusCols;

private:
        bool m_toInitialise;
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
