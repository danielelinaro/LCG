#include "probability_estimator.h"
#include <math.h>

lcg::Entity* ProbabilityEstimatorFactory(string_dict& args)
{
        uint id;
        double tau, initialProbability, stimulationFrequency, window;
        id = lcg::GetIdFromDictionary(args);
        if (! lcg::CheckAndExtractDouble(args, "tau", &tau) ||
            ! lcg::CheckAndExtractDouble(args, "stimulationFrequency", &stimulationFrequency) ||            
            ! lcg::CheckAndExtractDouble(args, "window", &window)) {
                lcg::Logger(lcg::Critical, "ProbabilityEstimator(%d): Unable to build.\n", id);
                return NULL;
                
        }
        if (! lcg::CheckAndExtractDouble(args, "initialProbability", &initialProbability))
                initialProbability = 0.5;
        return new lcg::ProbabilityEstimator(tau, stimulationFrequency, window, initialProbability, id);
        
}

namespace lcg {

ProbabilityEstimator::ProbabilityEstimator(double tau, double stimulationFrequency, double window, double initialProbability, uint id)
        : Entity(id)
{
        if (tau <= 0)
                throw "Tau must be positive";
        if (stimulationFrequency <= 0)
                throw "The stimulation frequency must be positive";
        if (window <= 0)
                throw "The window for spike detection must be positive";
        PE_TAU = tau;
        PE_P0 = initialProbability;
        PE_F = stimulationFrequency;
        PE_T = 1./stimulationFrequency;
        PE_WNDW = window;
        setName("ProbabilityEstimator");
        setUnits("1");
}

bool ProbabilityEstimator::initialise()
{
        m_tPrevSpike = 0.0;
        m_tPrevStim = 0.0;
        m_delay = 0.0;
        m_probability = PE_P0;
        m_flag = false;
        return true;
}

void ProbabilityEstimator::step()
{
        m_delay -= GetGlobalDt();
        if (m_flag && m_delay <= 0.0) {
                double weight;
                int spike;
                if (fabs(m_tPrevSpike - m_tPrevStim) < PE_WNDW) // the spike arrived within the window time after the stimulation
                        spike = 1;
                else
                        spike = 0;
                weight = exp(-PE_T/PE_TAU);
                m_probability = (1-weight)*spike + weight*m_probability;
                emitTrigger();
                Logger(Important, "ProbabilityEstimator(%d): estimated probability: %g\n", id(), m_probability);
                m_flag = false;
        }
}

double ProbabilityEstimator::output()
{
        return m_probability;
}

void ProbabilityEstimator::handleEvent(const Event *event)
{
	switch(event->type())
	{
                case SPIKE:
                        Logger(Important, "ProbabilityEstimator(%d): received spike at t = %g\n", id(), GetGlobalTime());
                        m_tPrevSpike = event->time();
                        break;
                case TRIGGER:
                        Logger(Debug, "ProbabilityEstimator(%d): received trigger at t = %g\n", id(), GetGlobalTime());
                        m_tPrevStim = event->time();
                        m_delay = PE_WNDW;
                        m_flag = true;
                        break;
                default:
                        Logger(Important, "ProbabilityEstimator received an unhandled event.\n");
                        break;
	}
}

void ProbabilityEstimator::emitTrigger() const
{
        emitEvent(new TriggerEvent(this));
}

} // namespace lcg

