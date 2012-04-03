#include <sstream>
#include <boost/filesystem.hpp>
#include "stimulus_generator.h"
#include "generate_trial.h"

namespace fs = boost::filesystem;

dynclamp::Entity* StimulusFactory(dictionary& args)
{
        uint id;
        std::string filename;
        id = dynclamp::GetIdFromDictionary(args);
        if ( ! dynclamp::CheckAndExtractValue(args, "filename", filename))
                return new dynclamp::generators::Stimulus(id);
        return new dynclamp::generators::Stimulus(filename.c_str(), id);
}

namespace dynclamp {

namespace generators {

Stimulus::Stimulus(uint id)
        : Generator(id), m_stimulus(NULL), m_stimulusMetadata(NULL), m_stimulusLength(0)
{}

Stimulus::Stimulus(const char *stimulusFile, uint id)
        : Generator(id), m_stimulus(NULL), m_stimulusMetadata(NULL), m_stimulusLength(0)
{
        setFilename(stimulusFile);
}

Stimulus::~Stimulus()
{
        freeMemory();
}

void Stimulus::initialise()
{
        m_position = 0;
}

bool Stimulus::setFilename(const char *filename)
{
        bool retval = true;
        int i, j, flag;
        double **metadata;

        if (!fs::exists(filename)) {
                Logger(Critical, "%s: no such file.\n", filename);
                return false;
        }
        strncpy(m_filename, filename, FILENAME_MAXLEN);

        freeMemory();

        flag = generate_trial(m_filename, GetLoggingLevel() <= Debug,
                              0, NULL, &m_stimulus, &m_stimulusLength,
                              1.0/GetGlobalDt(), GetGlobalDt());

        if (flag == -1) {
                if (m_stimulus != NULL)
                        free(m_stimulus);
                Logger(Critical, "Error in <generate_trial>\n");
                return false;
        }

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
        
        if (readmatrix(m_filename, metadata, &m_stimulusRows, &m_stimulusCols) == -1) {
                Logger(Critical, "Unable to parse file [%s].\n", m_filename);
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

double Stimulus::duration() const
{
        return stimulusLength() * GetGlobalDt();
}

uint Stimulus::stimulusLength() const
{
        return m_stimulusLength;
}

bool Stimulus::hasNext() const
{
        //return m_position < m_stimulusLength-1;
        return true;
}

double Stimulus::output() const
{
        if (m_position < m_stimulusLength)
                return m_stimulus[m_position];
        return 0.0;
}

void Stimulus::step()
{
        m_position++;
}

bool Stimulus::hasMetadata(size_t *ndims) const
{
        *ndims = 2;
        return true;
}

const double* Stimulus::metadata(size_t *dims, char *label) const
{
        sprintf(label, "Stimulus_Matrix");
        dims[0] = m_stimulusRows;
        dims[1] = m_stimulusCols;
        return m_stimulusMetadata;
}

void Stimulus::freeMemory()
{
        if (m_stimulus != NULL) {
                delete m_stimulus;
                delete m_stimulusMetadata;
                m_stimulus = NULL;
                m_stimulusMetadata = NULL;
        }
}

} // namespace generators

} // namespace dynclamp

