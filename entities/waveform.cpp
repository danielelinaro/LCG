#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include "waveform.h"
#include "generate_trial.h"
#include "events.h"

lcg::Entity* WaveformFactory(string_dict& args)
{
        uint id;
        bool triggered, emitEvent;
        std::string filename, units;
        const char *filenamePtr;
        id = lcg::GetIdFromDictionary(args);
        if (lcg::CheckAndExtractValue(args, "filename", filename))
                filenamePtr = filename.c_str();
        else
                filenamePtr = NULL;
        if (!lcg::CheckAndExtractBool(args, "triggered", &triggered))
                triggered = false;
        if (!lcg::CheckAndExtractValue(args, "units", units))
                units = "N/A";
        if (!lcg::CheckAndExtractBool(args, "emitEvent", &emitEvent))
                emitEvent = true;
        return new lcg::generators::Waveform(filenamePtr, triggered, units, emitEvent, id);
}

namespace lcg {

namespace generators {

Waveform::Waveform(const char *stimulusFile, bool triggered, const std::string& units, bool emitEventOnEnd, uint id)
        : Generator(id), m_triggered(triggered), m_emitEventOnEnd(emitEventOnEnd)
{
        setName("Waveform");
        setUnits(units);
        if (stimulusFile && strlen(stimulusFile))
                strncpy(m_stimulusFile, stimulusFile, FILENAME_MAXLEN);
        else
                m_stimulusFile[0] = 0;
        m_stimulus = new Stimulus(GetGlobalDt(), m_stimulusFile);
}

Waveform::~Waveform()
{
		delete m_stimulus;
}

bool Waveform::initialise()
{
	m_stimulus->setStimulusFile(m_stimulusFile);
	m_eventSent = false;
        if (!m_triggered) {
                m_position = 0;
        }
        else {
                m_position = m_stimulus->length() + 1;
                Logger(Debug, "Waveform is waiting for events.\n");
        }
        return true;
}

const char* Waveform::stimulusFile() const
{
        return m_stimulusFile;
}

bool Waveform::setStimulusFile(const char *stimulusFile)
{
        strncpy(m_stimulusFile, stimulusFile, FILENAME_MAXLEN);
	return m_stimulus->setStimulusFile(m_stimulusFile);
}

double Waveform::duration() const
{
        return m_stimulus->duration();
}

uint Waveform::stimulusLength() const
{
        return m_stimulus->length();
}

bool Waveform::hasNext() const
{
        return true;
}

void Waveform::step()
{
        m_position++;
}

bool Waveform::hasMetadata(size_t *ndims) const
{
        *ndims = 2;
        return true;
}

const double* Waveform::metadata(size_t *dims, char *label) const
{
        sprintf(label, "Stimulus_Matrix");
        return m_stimulus->metadata(&dims[0], &dims[1]);
}

/**
 * Note: a RESET event is sent when the waveform ends.
 */
double Waveform::output()
{ 
        if (m_position < m_stimulus->length())
                return m_stimulus->at(m_position);
        if (m_position == m_stimulus->length() && m_emitEventOnEnd && !m_eventSent) {
		Logger(Debug, "Waveform: emitting event at t = %lf seconds.\n", GetGlobalTime());
                emitEvent(new TriggerEvent(this));
		m_eventSent = true;
	}
        return m_stimulus->at(m_stimulus->length()-1);
}

void Waveform::handleEvent(const Event *event)
{
        switch(event->type()) {
        case TRIGGER:
                if (m_triggered && m_position >= m_stimulus->length()){
                    Logger(Debug, "Waveform: triggered by event.\n");
                    reset();
                }
                break;
        default:
                Logger(Important, "Waveform: unknown event (%d) type.\n",event->type());
        }
}

void Waveform::reset()
{
        m_position = 0;
		m_eventSent = false;
}

void Waveform::terminate()
{
        m_position = m_stimulus->length();
}

} // namespace generators

} // namespace lcg

