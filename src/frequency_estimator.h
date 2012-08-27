#ifndef FREQUENCY_ESTIMATOR_H
#define FREQUENCY_ESTIMATOR_H

#include "entity.h"
#include "utils.h"

#define FE_TAU m_parameters[0]
#define FE_INITIAL_F m_parameters[1]

namespace dynclamp {

class FrequencyEstimator : public Entity {
public:
        FrequencyEstimator(double tau, double initialFrequency = 0.0, uint id = GetId());
        void setTau(double tau);
        double tau() const;
        virtual bool initialise();
        virtual void step();
        virtual double output() const;
        virtual void handleEvent(const Event *event);
protected:
        void emitTrigger() const;
private:
        double m_tPrevSpike;
        double m_frequency;
};

} // namespace dynclamp

/***
 *   FACTORY METHODS
 ***/
#ifdef __cplusplus
extern "C" {
#endif

dynclamp::Entity* FrequencyEstimatorFactory(dictionary& args);
        
#ifdef __cplusplus
}
#endif

#endif

