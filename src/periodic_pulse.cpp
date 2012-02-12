#include "periodic_pulse.h"

namespace dynclamp {

namespace generators {

PeriodicPulse::PeriodicPulse(double frequency, double amplitude, double duration, uint id, double dt)
        : Generator(id,dt), m_output(0.0), m_tNextPulse(1.0/frequency)
{
        if (frequency <= 0)
                throw "Periodic pulse: stimulation frequency must be greater than 0";
        if (duration <= 0)
                throw "Periodic pulse: the duration of the stimulation must be greater than 0";

        m_parameters.push_back(frequency);      // m_parameters[0] -> frequency
        m_parameters.push_back(amplitude);      // m_parameters[1] -> amplitude
        m_parameters.push_back(duration);       // m_parameters[2] -> duration
        m_parameters.push_back(1.0/frequency);  // m_parameters[3] -> period

        Logger(Debug, "---\nPeriodicPulse:\n\tFrequency: %g\n\tAmplitude: %g\n\tDuration: %g\n\tPeriod: %g\n---\n",
                        PP_FREQUENCY, PP_AMPLITUDE, PP_DURATION, PP_PERIOD);
}

bool PeriodicPulse::hasNext() const
{
        return true;
}

void PeriodicPulse::step()
{
        double now = GetGlobalTime();
        if (now >= m_tNextPulse) {
                if (m_output == 0.0) {
                        Logger(Debug, "Turning output ON @ t = %g s.\n", now);
                        m_output = PP_AMPLITUDE;
                }
                else if (now >= m_tNextPulse+PP_DURATION) {
                        Logger(Debug, "Turning output OFF @ t = %g s.\n", now);
                        m_tNextPulse += PP_PERIOD;
                        m_output = 0.0;
                }
        }
}

double PeriodicPulse::output() const
{
        return m_output;
}

double PeriodicPulse::frequency() const
{
        return PP_FREQUENCY;
}

double PeriodicPulse::period() const
{
        return PP_PERIOD;
}

double PeriodicPulse::amplitude() const
{
        return PP_AMPLITUDE;
}

double PeriodicPulse::duration() const
{
        return PP_DURATION;
}

void PeriodicPulse::setFrequency(double frequency)
{
        if (frequency > 0) {
                PP_FREQUENCY = frequency;
                PP_PERIOD = 1.0/frequency;
        }
}

void PeriodicPulse::setPeriod(double period)
{
        if (period > 0) {
                PP_PERIOD = period;
                PP_FREQUENCY = 1.0/period;
        }
}

void PeriodicPulse::setAmplitude(double amplitude)
{
        PP_AMPLITUDE = amplitude;
}

void PeriodicPulse::setDuration(double duration)
{
        if (duration > 0)
                PP_DURATION = duration;
}

} // namespace generators

} // namespace dynclamp

