#include <sys/stat.h>
#include <sstream>
#include "stimulus_generator.h"
#include "generate_trial.h"

dynclamp::Entity* StimulusFactory(dictionary& args)
{
        uint id;
        std::string filename;
        id = dynclamp::GetIdFromDictionary(args);
        if ( ! dynclamp::CheckAndExtractValue(args, "filename", filename))
                return NULL;
        return new dynclamp::generators::Stimulus(filename.c_str(), id);
}

namespace dynclamp {

namespace generators {

Stimulus::Stimulus(const char *stimulusFile, uint id)
        : Generator(id), m_stimulus(NULL), m_stimulusLength(0), m_position(0)
{
        double dt = GetGlobalDt();
        int i, j, flag;
        double **metadata;
        struct stat buf;

        // check for file existence
        if (stat(stimulusFile, &buf) != 0) {
                std::stringstream ss;
                ss << stimulusFile << ": no such file.";
                throw ss.str().c_str();
        }

        m_parameters.push_back(1.0/dt);         // m_parameters[0] -> sampling rate

        flag = generate_trial(stimulusFile, GetLoggingLevel() <= Debug,
                              0, NULL, &m_stimulus, &m_stimulusLength, m_parameters[0], dt);

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
        return stimulusLength() * GetGlobalDt();
}

uint Stimulus::stimulusLength() const
{
        return m_stimulusLength;
}

bool Stimulus::hasNext() const
{
        return m_position < m_stimulusLength-1;
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

} // namespace generators

} // namespace dynclamp

