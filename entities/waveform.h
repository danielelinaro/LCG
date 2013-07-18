#ifndef WAVEFORM_H
#define WAVEFORM_H

#include "entity.h"
#include "types.h"
#include "utils.h"
#include "generator.h"
#include "common.h"
#include "stimulus.h"

namespace lcg {

namespace generators {

class Waveform : public Generator {
public:
        Waveform(const char *stimulusFile = NULL, bool triggered = false,
                 const std::string& units = "N/A", uint id = GetId());
        virtual ~Waveform();

        const char* stimulusFile() const;
        bool setStimulusFile(const char *filename);
        virtual bool initialise();

        uint stimulusLength() const;
        virtual bool hasNext() const;

        virtual void step();
        virtual void terminate();

        virtual double output(); 
        virtual void handleEvent(const Event *event);
        virtual bool hasMetadata(size_t *ndims) const;
        virtual const double* metadata(size_t *dims, char *label) const;

        double duration() const;

protected:
        virtual void reset();

private:
        uint m_position;
        bool m_triggered;
        Stimulus *m_stimulus;
        char m_stimulusFile[FILENAME_MAXLEN];
        bool m_toInitialise;
};

} // namespace generators

} // namespace lcg

/***
 *   FACTORY METHODS
 ***/
#ifdef __cplusplus
extern "C" {
#endif

lcg::Entity* WaveformFactory(string_dict& args);
	
#ifdef __cplusplus
}
#endif

#endif

