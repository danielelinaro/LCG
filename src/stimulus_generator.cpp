#include <sys/stat.h>
#include <sstream>
#include "stimulus_generator.h"

dynclamp::Entity* StimulusFactory(dictionary& args)
{
        uint id;
        double dt;
        std::string filename;
        dynclamp::GetIdAndDtFromDictionary(args, &id, &dt);
        if ( ! dynclamp::CheckAndExtractValue(args, "filename", filename))
                return NULL;
        return new dynclamp::generators::Stimulus(filename.c_str(), id, dt);
}

namespace dynclamp {

namespace generators {

Stimulus::Stimulus(const char *stimulusFile, uint id, double dt)
        : Generator(id, dt), m_stimulus(NULL), m_stimulusLength(0), m_position(0)
{
        int i, j, flag;
        double **metadata;
        struct stat buf;

        // check for file existence
        if (stat(stimulusFile, &buf) != 0) {
                std::stringstream ss;
                ss << stimulusFile << ": no such file.";
                throw ss.str().c_str();
        }

        m_parameters.push_back(1.0/m_dt);         // m_parameters[0] -> sampling rate

        flag = generate_trial(stimulusFile, GetLoggingLevel() <= Debug,
                              0, NULL, &m_stimulus, &m_stimulusLength, m_parameters[0], m_dt);

        if (flag == -1) {
                if (m_stimulus != NULL)
                        free(m_stimulus);
                throw "Error in <generate_trial>";
        }

        metadata = new double*[MAXROWS];
        if (metadata == NULL)
                throw "Unable to allocate memory for <parsed_data> !";
        for (i=0; i<MAXROWS; i++)  { 
                metadata[i] = new double[MAXCOLS];
                if (metadata[i] == NULL)
                        throw "Unable to allocate memory for <parsed_data> !";
        }
        
        if (readmatrix(stimulusFile, metadata, &m_stimulusRows, &m_stimulusCols) == -1)
                throw "Unable to parse file.";

        m_stimulusMetadata = new double[m_stimulusRows * m_stimulusCols];
        for (i=0; i<m_stimulusRows; i++) {
                for (j=0; j<m_stimulusCols; j++) {
                        m_stimulusMetadata[i*m_stimulusCols + j] = metadata[i][j];
                        Logger(Debug, "%7.1lf ", m_stimulusMetadata[i*m_stimulusCols + j]);
                }
                Logger(Debug, "\n");
                delete metadata[i];
        }
        delete metadata;
}

Stimulus::~Stimulus()
{
        if (m_stimulus != NULL) {
                delete m_stimulus;
                delete m_stimulusMetadata;
        }
}


double Stimulus::duration() const
{
        return stimulusLength() * dt();
}

uint Stimulus::stimulusLength() const
{
        return m_stimulusLength;
}

bool Stimulus::hasNext() const
{
        return m_position < m_stimulusLength;
}

double Stimulus::output() const
{
        return m_stimulus[m_position];
}

void Stimulus::step()
{
        if (hasNext())
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

} // namespace generators

} // namespace dynclamp

