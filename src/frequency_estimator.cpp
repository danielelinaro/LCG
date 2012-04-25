#include "frequency_estimator.h"
#include <math.h>

dynclamp::Entity* FrequencyEstimatorFactory(dictionary& args)
{
        uint id;
        double tau;
        id = dynclamp::GetIdFromDictionary(args);
        if (! dynclamp::CheckAndExtractDouble(args, "tau", &tau)) {
                dynclamp::Logger(dynclamp::Critical, "Unable to build FrequencyEstimator.\n");
                return NULL;
                
        }
        return new dynclamp::FrequencyEstimator(tau, id);
        
}

namespace dynclamp {

FrequencyEstimator::FrequencyEstimator(double tau, uint id)
        : Entity(id)
{
        if (tau <= 0)
                throw "Tau must be positive";
        m_parameters.push_back(tau);
}

void FrequencyEstimator::setTau(double tau)
{
        if (tau <= 0)
                throw "Tau must be positive";
        FE_TAU = tau;
}

double FrequencyEstimator::tau() const
{
        return FE_TAU;
}

void FrequencyEstimator::initialise()
{
        m_tPrevSpike = 0.0;
        m_frequency = 0.0;
}

void FrequencyEstimator::step()
{}

double FrequencyEstimator::output() const
{
        return m_frequency;
}

void FrequencyEstimator::handleEvent(const Event *event)
{
        if (event->type() == SPIKE) {
                double now = event->time();
                if (m_tPrevSpike > 0) {
                        double isi, weight;
                        isi = now - m_tPrevSpike;
                        weight = exp(-isi/FE_TAU);
                        m_frequency = (1-weight)/isi + weight*m_frequency;
                        Logger(Debug, "Estimated frequency: %g\n", m_frequency);
                }
                m_tPrevSpike = now;
        }
}

void FrequencyEstimator::emitTrigger() const
{
        emitEvent(new TriggerEvent(this));
}

} // namespace dynclamp

