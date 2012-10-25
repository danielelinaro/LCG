#ifndef PROBABILITY_ESTIMATOR_H
#define PROBABILITY_ESTIMATOR_H

#include "entity.h"
#include "utils.h"

#define PE_TAU  m_parameters["tau"]
#define PE_P0   m_parameters["p0"]
#define PE_WNDW m_parameters["window"]
#define PE_F    m_parameters["frequency"]
#define PE_T    m_parameters["period"]

namespace dynclamp {

class ProbabilityEstimator : public Entity {
public:
        ProbabilityEstimator(double tau, double stimulationFrequency, double window,
                        double initialProbability = 0.5, uint id = GetId());
        virtual bool initialise();
        virtual void step();
        virtual double output();
        virtual void handleEvent(const Event *event);
protected:
        void emitTrigger() const;
private:
        double m_tPrevStim;
        double m_tPrevSpike;
        double m_delay;
        double m_probability;
        bool m_flag;
};

} // namespace dynclamp

/***
 *   FACTORY METHODS
 ***/
#ifdef __cplusplus
extern "C" {
#endif

dynclamp::Entity* ProbabilityEstimatorFactory(string_dict& args);
        
#ifdef __cplusplus
}
#endif

#endif

