#include "periodic_pulse.h"

namespace dynclamp {

namespace generators {

PeriodicPulse::PeriodicPulse(double frequency, double amplitude, uint id, double dt)
        : Generator(id,dt)
{
        m_parameters.push_back(frequency);
        m_parameters.push_back(amplitude);
}

bool PeriodicPulse::hasNext() const
{
        return true;
}

void PeriodicPulse::step()
{
}

double PeriodicPulse::output() const
{
        return 0.0;
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

} // namespace generators

} // namespace dynclamp

