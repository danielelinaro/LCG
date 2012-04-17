#include "frequency_clamp.h"
#include <math.h>

dynclamp::Entity* FrequencyClampFactory(dictionary& args)
{        
        uint id;
        double frequency, baselineCurrent, tau, gp, gi, gd;

        id = dynclamp::GetIdFromDictionary(args);

        if ( ! dynclamp::CheckAndExtractDouble(args, "baselineCurrent", &baselineCurrent) ||
             ! dynclamp::CheckAndExtractDouble(args, "tau", &tau) ||
             ! dynclamp::CheckAndExtractDouble(args, "gp", &gp) ||
             ! dynclamp::CheckAndExtractDouble(args, "gi", &gi) ||
             ! dynclamp::CheckAndExtractDouble(args, "gd", &gd)) {
                dynclamp::Logger(dynclamp::Critical, "Unable to build FrequencyClamp.\n");
                return NULL;
        }

        if ( ! dynclamp::CheckAndExtractDouble(args, "frequency", &frequency))
                return new dynclamp::generators::FrequencyClamp(baselineCurrent, tau, gp, gi, gd, id);

        return new dynclamp::generators::FrequencyClamp(frequency, baselineCurrent, tau, gp, gi, gd, id);

}

namespace dynclamp {

namespace generators {

FrequencyClamp::FrequencyClamp(double baselineCurrent, double tau,
                double gp, double gi, double gd, uint id)
        : Generator(id), m_constantFrequency(false)
{
        m_parameters.push_back(baselineCurrent);// m_parameters[0] -> baseline current
        m_parameters.push_back(tau);            // m_parameters[1] -> time constant
        m_parameters.push_back(gp);             // m_parameters[2] -> proportional gain
        m_parameters.push_back(gi);             // m_parameters[3] -> integral gain
        m_parameters.push_back(gd);             // m_parameters[4] -> derivative gain
}

FrequencyClamp::FrequencyClamp(double frequency, double baselineCurrent, double tau,
                double gp, double gi, double gd, uint id)
        : Generator(id), m_constantFrequency(true)
{
        m_parameters.push_back(baselineCurrent);// m_parameters[0] -> baseline current
        m_parameters.push_back(tau);            // m_parameters[1] -> time constant
        m_parameters.push_back(gp);             // m_parameters[2] -> proportional gain
        m_parameters.push_back(gi);             // m_parameters[3] -> integral gain
        m_parameters.push_back(gd);             // m_parameters[4] -> derivative gain
        m_parameters.push_back(frequency);      // m_parameters[5] -> frequency
}

bool FrequencyClamp::hasNext() const
{
        return true;
}

void FrequencyClamp::initialise()
{
        m_estimatedFrequency = 0.0;
        m_tPrevSpike = 0.0;
        if (m_constantFrequency)
                m_errp = FC_F;
        else
                m_errp = 0.0;
        m_erri = 0.0;
        m_errd = 0.0;
        m_errorpPrev = 0.0;
        m_current = FC_BASELINE;
}

void FrequencyClamp::step()
{}

double FrequencyClamp::output() const
{
        return m_current;
}

void FrequencyClamp::handleEvent(const Event *event)
{
        double now = event->time();
        if (m_tPrevSpike > 0) {
                double isi, weight;
                isi = now - m_tPrevSpike;
                weight = exp(-isi/FC_TAU);
                m_estimatedFrequency = (1-weight)/isi + weight*m_estimatedFrequency;
                if (m_constantFrequency)
                        m_errp = FC_F - m_estimatedFrequency;
                else
                        // a waveform generator must be connected to this entity
                        m_errp = m_inputs[0] - m_estimatedFrequency;
                m_erri += m_errp;
                m_errd = m_errp - m_errorpPrev;
                m_errorpPrev = m_errp;
                m_current = FC_BASELINE + FC_GP*m_errp + FC_GI*m_erri + FC_GD*m_errd;
                Logger(Info, "%g %g %g\n", now, m_current, m_estimatedFrequency);
        }
        m_tPrevSpike = now;
}

} // namespace generators

} // namespace dynclamp
