#include <math.h>
#include "periodic_pulse.h"
#include "engine.h"

dynclamp::Entity* PeriodicPulseFactory(dictionary& args)
{
        uint id;
        double frequency, duration, amplitude;
        double probability, tau, gp, gi, gd;

        id = dynclamp::GetIdFromDictionary(args);

        if ( ! dynclamp::CheckAndExtractDouble(args, "frequency", &frequency) ||
             ! dynclamp::CheckAndExtractDouble(args, "duration", &duration) ||
             ! dynclamp::CheckAndExtractDouble(args, "amplitude", &amplitude)) {
                dynclamp::Logger(dynclamp::Critical, "Unable to build PeriodicPulse.\n");
                return NULL;
        }

        if ( ! dynclamp::CheckAndExtractDouble(args, "probability", &probability))
                return new dynclamp::generators::PeriodicPulse(frequency, duration, amplitude, id);

        if ( ! dynclamp::CheckAndExtractDouble(args, "tau", &tau) ||
             ! dynclamp::CheckAndExtractDouble(args, "gp", &gp)) {
                dynclamp::Logger(dynclamp::Critical, "Unable to build PeriodicPulse.\n");
                return NULL;
        }

        if ( ! dynclamp::CheckAndExtractDouble(args, "gi", &gi))
                gi = 0.0;
        
        if ( ! dynclamp::CheckAndExtractDouble(args, "gd", &gd))
                gd = 0.0;

        return new dynclamp::generators::PeriodicPulse(frequency, duration, amplitude, 
                        probability, tau, gp, gi, gd, id);
}

namespace dynclamp {

namespace generators {

PeriodicPulse::PeriodicPulse(double frequency, double duration, double amplitude, uint id)
        : Generator(id), m_clamp(false)
{
        if (frequency <= 0)
                throw "Periodic pulse: stimulation frequency must be greater than 0";
        if (duration <= 0)
                throw "Periodic pulse: the duration of the stimulation must be greater than 0";

        m_parameters.push_back(frequency);      // m_parameters[0] -> frequency
        m_parameters.push_back(duration);       // m_parameters[1] -> duration
        m_parameters.push_back(amplitude);      // m_parameters[2] -> amplitude
        m_parameters.push_back(1.0/frequency);  // m_parameters[3] -> period

        setName("PeriodicPulse");
        setUnits("pA");

        Logger(Debug, "---\nPeriodicPulse:\n\tFrequency: %g\n\tAmplitude: %g\n\tDuration: %g\n\tPeriod: %g\n---\n",
                        PP_FREQUENCY, PP_AMPLITUDE, PP_DURATION, PP_PERIOD);
        Logger(Debug, "m_parameters.size() = %d\n", m_parameters.size());
}
        
PeriodicPulse::PeriodicPulse(double frequency, double duration, double amplitude, 
                             double probability, double tau, double gp, double gi, double gd,
                             uint id)
        : Generator(id), m_clamp(true)
{
        if (frequency <= 0)
                throw "Periodic pulse: stimulation frequency must be greater than 0";
        if (duration <= 0)
                throw "Periodic pulse: the duration of the stimulation must be greater than 0";

        m_parameters.push_back(frequency);      // m_parameters[0] -> frequency
        m_parameters.push_back(duration);       // m_parameters[1] -> duration
        m_parameters.push_back(amplitude);      // m_parameters[2] -> amplitude
        m_parameters.push_back(1.0/frequency);  // m_parameters[3] -> period
        m_parameters.push_back(probability);    // m_parameters[4] -> clamp probability
        m_parameters.push_back(tau);            // m_parameters[5] -> time constant
        m_parameters.push_back(gp);             // m_parameters[6] -> proportional gain
        m_parameters.push_back(gi);             // m_parameters[7] -> integral gain
        m_parameters.push_back(gd);             // m_parameters[8] -> derivative gain

        setName("PeriodicPulse");
        setUnits("pA");

        Logger(Info, "---\nPeriodicPulse:\n\tFrequency: %g Hz\n\tAmplitude: %g pA\n\tDuration: %g sec\n\tPeriod: %g sec\n---\n",
                        PP_FREQUENCY, PP_AMPLITUDE, PP_DURATION, PP_PERIOD);
        Logger(Info, "---\nPeriodicPulse:\n\tClamp prob: %g\n\tTau: %g sec\n\tgp: %g\n\tgi: %g\n\tgd = %g\n---\n",
                PP_PROB, PP_TAU, PP_GP, PP_GI, PP_GD);
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
        m_estimatedProbability = 0.5;
        m_errp = m_erri = m_errd = 0.0;
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
        if (m_clamp && now >= m_tUpdate) {
                int s = 0;
                double weight;
                if (m_tLastSpike > m_tLastPulse) {
                        s = 1;
                        Logger(Debug, "The last stimulation (@ t = %g sec) elicited a spike @ t = %g sec.\n", m_tLastPulse, m_tLastSpike);
                }
                weight = exp(-PP_PERIOD/PP_TAU);
                m_estimatedProbability = (1-weight)*s + weight*m_estimatedProbability;
                m_errp = PP_PROB - m_estimatedProbability;
                m_erri += m_errp;
                m_errd = m_errp - m_errpPrev;
                m_errpPrev = m_errp;
                m_amplitude = PP_AMPLITUDE + PP_GP*m_errp + PP_GI*m_erri + PP_GD*m_errd;
                Logger(Info, "%g %g %g\n", now, m_estimatedProbability, m_amplitude);
                m_tUpdate += PP_PERIOD;
        }
}

double PeriodicPulse::output() const
{
        return m_output;
}

void PeriodicPulse::handleEvent(const Event* event)
{
        m_tLastSpike = event->time();
        Logger(Debug, "Last spike was @ t = %g sec.\n", m_tLastSpike);
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

