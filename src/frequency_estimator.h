#ifndef FREQUENCY_ESTIMATOR_H
#define FREQUENCY_ESTIMATOR_H

#include "entity.h"
#include "utils.h"

#define FE_TAU m_parameters["tau"]
#define FE_F0  m_parameters["f0"]

namespace dynclamp {

class FrequencyEstimator : public Entity {
public:
        FrequencyEstimator(double tau, double initialFrequency = 0.0, uint id = GetId());
        void setTau(double tau);
        void changeState();
	bool state();
        virtual bool initialise();
        virtual void step();
        virtual double output();
        virtual void handleEvent(const Event *event);
protected:
        void emitTrigger() const;
private:
	bool m_state;
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

dynclamp::Entity* FrequencyEstimatorFactory(string_dict& args);
        
#ifdef __cplusplus
}
#endif

#endif

