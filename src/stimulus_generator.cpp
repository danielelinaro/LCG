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
        int flag;
        struct stat buf;

        // check for file existence
        if (stat(stimulusFile, &buf) != 0) {
                std::stringstream ss;
                ss << stimulusFile << ": no such file.";
                throw ss.str().c_str();
        }

        m_parameters.push_back(1.0/dt);         // m_parameters[0] -> sampling rate

        flag = generate_trial(stimulusFile, GetLoggingLevel() <= Debug,
                              0, NULL, &m_stimulus, &m_stimulusLength, m_parameters[0], m_dt);

        if (flag == -1) {
                if (m_stimulus != NULL)
                        free(m_stimulus);
                throw "Error in <generate_trial>.";
        }
}

Stimulus::~Stimulus()
{
        if (m_stimulus != NULL)
                free(m_stimulus);
}


INT Stimulus::stimulusLength() const
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
        m_position++;
}

} // namespace generators

} // namespace dynclamp

