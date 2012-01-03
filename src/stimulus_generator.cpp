#include <sys/stat.h>
#include <sstream>
#include "stimulus_generator.h"

namespace dynclamp {

StimulusGenerator::StimulusGenerator(const char *stimulusFile, uint id, double dt)
        : Entity(id, dt), m_srate(1.0/dt), m_stimulus(NULL), m_stimulusLength(0), m_position(0)
{
        int flag;
        struct stat buf;

        if (stat(stimulusFile, &buf) != 0) {
                std::stringstream ss;
                ss << stimulusFile << ": no such file.";
                throw ss.str().c_str();
        }

        flag = generate_trial(stimulusFile, GetLoggingLevel() <= Debug,
                              0, NULL, &m_stimulus, &m_stimulusLength, m_srate, m_dt);

        if (flag == -1) {
                if (m_stimulus != NULL)
                        free(m_stimulus);
                throw "Error in <generate_trial>.";
        }
}

StimulusGenerator::~StimulusGenerator()
{
        if (m_stimulus != NULL)
                free(m_stimulus);
}


INT StimulusGenerator::stimulusLength() const
{
        return m_stimulusLength;
}

bool StimulusGenerator::hasNext() const
{
        return m_position < m_stimulusLength;
}

double StimulusGenerator::output()
{
        double retval = m_stimulus[m_position];
        m_position++;
        return retval;
}

} // namespace dynclamp

