#include "frequency_clamp.h"
#include <math.h>

namespace dynclamp {

namespace generators {

FrequencyClamp::FrequencyClamp(double frequency, double baselineCurrent, double tau,
                double gp, double gi, double gd, uint id, double dt)
        : Generator(id, dt), m_estimatedFrequency(0.0), m_tPrevSpike(0.0),
          m_errp(frequency), m_erri(0.0), m_errd(0.0), m_errorpPrev(0.0),
          m_current(baselineCurrent)
{
        m_parameters.push_back(frequency);      // m_parameters[0] -> frequency
        m_parameters.push_back(baselineCurrent);// m_parameters[1] -> baseline current
        m_parameters.push_back(tau);            // m_parameters[2] -> time constant
        m_parameters.push_back(gp);             // m_parameters[3] -> proportional gain
        m_parameters.push_back(gi);             // m_parameters[4] -> integral gain
        m_parameters.push_back(gd);             // m_parameters[5] -> derivative gain
}

bool FrequencyClamp::hasNext() const
{
        return true;
}

void FrequencyClamp::step()
{}

double FrequencyClamp::output() const
{
        return m_current;
}

void FrequencyClamp::handleEvent(const Event *event)
{
        double now = GetGlobalTime();
        if (m_tPrevSpike > 0) {
                double isi, weight;
                isi = now - m_tPrevSpike;
                weight = exp(-isi/FC_TAU);
                m_estimatedFrequency = (1-weight)/isi + weight*m_estimatedFrequency;
                m_errp = FC_F - m_estimatedFrequency;
                m_erri += m_errp;
                m_errd = m_errp - m_errorpPrev;
                m_errorpPrev = m_errp;
                m_current = FC_BASELINE + FC_GP*m_errp + FC_GI*m_erri + FC_GD*m_errd;
                Logger(Debug, "%g %g %g\n", now, m_current, m_estimatedFrequency);
        }
        m_tPrevSpike = now;
}

} // namespace generators

} // namespace dynclamp
