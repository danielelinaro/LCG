#include <math.h>
#include "periodic_pulse.h"
#include "engine.h"

dynclamp::Entity* PeriodicPulseFactory(string_dict& args)
{
        uint id;
        double frequency, duration, amplitude;
        //double probability, tau, gp, gi, gd;

        id = dynclamp::GetIdFromDictionary(args);

        if ( ! dynclamp::CheckAndExtractDouble(args, "frequency", &frequency) ||
             ! dynclamp::CheckAndExtractDouble(args, "duration", &duration) ||
             ! dynclamp::CheckAndExtractDouble(args, "amplitude", &amplitude)) {
                dynclamp::Logger(dynclamp::Critical, "Unable to build PeriodicPulse.\n");
                return NULL;
        }

        return new dynclamp::generators::PeriodicPulse(frequency, duration, amplitude, id);
}

namespace dynclamp {

namespace generators {

PeriodicPulse::PeriodicPulse(double frequency, double duration, double amplitude, uint id)
        : Generator(id)
{
        if (frequency <= 0)
                throw "Periodic pulse: stimulation frequency must be greater than 0";
        if (duration <= 0)
                throw "Periodic pulse: the duration of the stimulation must be greater than 0";

        PP_FREQUENCY = frequency;
        PP_DURATION = duration;
        PP_AMPLITUDE = amplitude;
        PP_PERIOD = 1.0 / frequency;
        m_period = PP_PERIOD;

        setName("PeriodicPulse");
        setUnits("pA");

        Logger(Debug, "---\nPeriodicPulse:\n\tFrequency: %g\n\tAmplitude: %g\n\tDuration: %g\n\tPeriod: %g\n---\n",
                        PP_FREQUENCY, PP_AMPLITUDE, PP_DURATION, PP_PERIOD);
        Logger(Debug, "m_parameters.size() = %d\n", m_parameters.size());
}
        
bool PeriodicPulse::initialise()
{
        m_output = 0.0;
        m_amplitude = PP_AMPLITUDE;
        m_tLastPulse = 0.0;
        m_tNextPulse = 1.0 / PP_FREQUENCY;
        m_tUpdate = 1.0 / PP_FREQUENCY + PP_DURATION + PP_INTERVAL;
        m_tLastSpike = -PP_INTERVAL;
        return true;
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
                        m_output = m_amplitude;
                        m_tLastPulse = m_tNextPulse;
                }
                else if (now >= m_tNextPulse+PP_DURATION) {
                        Logger(Debug, "Turning output OFF @ t = %g s.\n", now);
                        m_tNextPulse += PP_PERIOD;
                        m_output = 0.0;
                }
        }
}

double PeriodicPulse::output()
{
        return m_output;
}

void PeriodicPulse::handleEvent(const Event* event)
{
        m_tLastSpike = event->time();
        Logger(Debug, "Last spike was @ t = %g sec.\n", m_tLastSpike);
}

double PeriodicPulse::period() const
{
        return m_period;
}

void PeriodicPulse::setFrequency(double frequency)
{
        if (frequency > 0) {
                PP_FREQUENCY = frequency;
                PP_PERIOD = 1.0/frequency;
                m_period = PP_PERIOD;
        }
}

void PeriodicPulse::setPeriod(double period)
{
        if (period > 0) {
                PP_PERIOD = period;
                m_period = PP_PERIOD;
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

