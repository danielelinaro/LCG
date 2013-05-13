#include <sstream>
#include <boost/filesystem.hpp>
#include "waveform.h"
#include "generate_trial.h"
#include "events.h"
#include "engine.h"
namespace fs = boost::filesystem;

lcg::Entity* WaveformFactory(string_dict& args)
{
        uint id;
        bool triggered;
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
        return new lcg::generators::Waveform(filenamePtr, triggered, units, id);
}

namespace lcg {

namespace generators {

Waveform::Waveform(const char *stimulusFile, bool triggered, const std::string& units, uint id)
        : Generator(id), m_stimulus(NULL), m_stimulusMetadata(NULL),
          m_stimulusLength(0), m_triggered(triggered), m_toInitialise(true)
{
        if (stimulusFile != NULL) {
            if (!setStimulusFile(stimulusFile))
                    throw "missing stimulus file";
        }
        setName("Waveform");
        setUnits(units);
}

Waveform::~Waveform()
{
        freeMemory();
}

bool Waveform::initialise()
{
        if (m_toInitialise && !parseStimulusFile()) {
                Logger(Critical, "Unable to parse stimulus file [%s].\n", m_stimulusFile);
                return false;
        }
        // parse again the file in the next calls to initialise
        m_toInitialise = true;

        if (!m_triggered) {
                m_position = 0;
        }
        else {
                m_position = m_stimulusLength + 1;
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
        if (!fs::exists(stimulusFile)) {
                Logger(Critical, "%s: no such file.\n", stimulusFile);
                return false;
        }
        strncpy(m_stimulusFile, stimulusFile, FILENAME_MAXLEN);
        // parse the stimulus file here in order to have the metadata
        bool flag = parseStimulusFile();
        // the first call to initialise should not parse again the file
        m_toInitialise = !flag;
        return flag;
}

bool Waveform::parseStimulusFile()
{
        bool retval = true;
        int i, j, flag;
        double **metadata;

        freeMemory();

        flag = generate_trial(m_stimulusFile, GetLoggingLevel() <= Debug,
                              0, NULL, &m_stimulus, &m_stimulusLength,
                              1.0/GetGlobalDt(), GetGlobalDt());
        Logger(Debug,"Passed %lf - %lf to generate_trial.\n",1.0/GetGlobalDt(),GetGlobalDt());
        if (flag == -1) {
                if (m_stimulus != NULL)
                        free(m_stimulus);
                Logger(Critical, "Error in <generate_trial>\n");
                return false;
        }
        Logger(Debug,"The number of points in the stimulus is: %d, which will last for: %lf (s).\n",m_stimulusLength,m_stimulusLength*GetGlobalDt());
        metadata = new double*[MAXROWS];
        if (metadata == NULL) {
                Logger(Critical, "Unable to allocate memory for <parsed_data>.\n");
                return false;
        }
        for (i=0; i<MAXROWS; i++)  { 
                metadata[i] = new double[MAXCOLS];
                if (metadata[i] == NULL) {
                        Logger(Critical, "Unable to allocate memory for <parsed_data>.\n");
                        for (j=0; j<i; i++)
                                delete metadata[j];
                        delete metadata;
                        return false;
                }
        }
        
        if (readmatrix(m_stimulusFile, metadata, &m_stimulusRows, &m_stimulusCols) == -1) {
                Logger(Critical, "Unable to parse file [%s].\n", m_stimulusFile);
                retval = false;
                goto endSetFilename;
        }

        m_stimulusMetadata = new double[m_stimulusRows * m_stimulusCols];
        for (i=0; i<m_stimulusRows; i++) {
                for (j=0; j<m_stimulusCols; j++) {
                        m_stimulusMetadata[i*m_stimulusCols + j] = metadata[i][j];
                        Logger(Debug, "%7.1lf ", m_stimulusMetadata[i*m_stimulusCols + j]);
                }
                Logger(Debug, "\n");
        }

endSetFilename:
        for (i=0; i<MAXROWS; i++)
                delete metadata[i];
        delete metadata;

        return retval;
}

double Waveform::duration() const
{
        return stimulusLength() * GetGlobalDt();
}

uint Waveform::stimulusLength() const
{
        return m_stimulusLength;
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
        dims[0] = m_stimulusRows;
        dims[1] = m_stimulusCols;
        return m_stimulusMetadata;
}

void Waveform::freeMemory()
{
        if (m_stimulus != NULL) {
                delete m_stimulus;
                delete m_stimulusMetadata;
                m_stimulus = NULL;
                m_stimulusMetadata = NULL;
        }
}

/**
 * Note: a RESET event is sent when the waveform ends.
 */
double Waveform::output()
{
        if (m_position < m_stimulusLength)
                return m_stimulus[m_position];
        if (m_position == m_stimulusLength)
            emitEvent(new ResetEvent(this));
        return 0.0;
}

void Waveform::handleEvent(const Event *event)
{
        switch(event->type()) {
        case TRIGGER:
                if (m_triggered && m_position >= m_stimulusLength){
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
}

void Waveform::terminate()
{
        m_position = m_stimulusLength;
}

} // namespace generators

} // namespace lcg

