#include "frequency_estimator.h"
#include "engine.h"
#include <math.h>

lcg::Entity* FrequencyEstimatorFactory(string_dict& args)
{
        uint id;
        double tau;
        double initialFrequency;
        id = lcg::GetIdFromDictionary(args);
        if (! lcg::CheckAndExtractDouble(args, "tau", &tau)) {
                lcg::Logger(lcg::Critical, "FrequencyEstimator(%d): Unable to build. Need to specify tau.\n", id);
                return NULL;
                
        }
        if (! lcg::CheckAndExtractDouble(args, "initialFrequency", &initialFrequency))
                initialFrequency = 0.0;
        else
                lcg::Logger(lcg::Info, "FrequencyEstimator(%d): Initialized at %f Hz.\n", id, initialFrequency);
        return new lcg::FrequencyEstimator(tau,initialFrequency, id);
        
}

namespace lcg {

FrequencyEstimator::FrequencyEstimator(double tau, double initialFrequency, uint id)
        : Entity(id)
{
        if (tau <= 0)
                throw "Tau must be positive";
        m_parameters["tau"] = tau;
        m_parameters["f0"] = initialFrequency;
        setName("FrequencyEstimator");
        setUnits("Hz");
}

void FrequencyEstimator::setTau(double tau)
{
        if (tau <= 0)
                throw "FrequencyEstimator: Tau must be positive.";
        FE_TAU = tau;
}

bool FrequencyEstimator::initialise()
{
        m_tPrevSpike = 0.0;
        m_frequency = FE_F0;
	m_state = true;
        return true;
}

void FrequencyEstimator::step()
{}

double FrequencyEstimator::output()
{
        return m_frequency;
}

bool FrequencyEstimator::state()
{
        return m_state;
}

void FrequencyEstimator::changeState()
{
        m_state = !m_state;
}

void FrequencyEstimator::handleEvent(const Event *event)
{
	double now = event->time();
	switch(event->type())
	{
        	case SPIKE:
                        if(m_tPrevSpike > 0 && m_state) {
                                double isi, weight;
                                isi = now - m_tPrevSpike;
                                weight = exp(-isi/FE_TAU);
                                m_frequency = (1-weight)/isi + weight*m_frequency;
                                emitTrigger();
                                Logger(Debug, "FrequencyEstimator(%d): Estimated frequency: %g\n", id(), m_frequency);
                        }
                        m_tPrevSpike = now; // Updates last spike also when TOGGLE is off.
        		break;
                case TOGGLE:
                        changeState();
                        Logger(Debug, "FrequencyEstimator(%d): Toggled at %9.3f.\n", id(), GetGlobalTime());                
                        break;
	}
}

void FrequencyEstimator::emitTrigger() const
{
        emitEvent(new TriggerEvent(this));
}

} // namespace lcg

