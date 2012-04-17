#ifndef CONDUCTANCE_STIMULUS_H
#define CONDUCTANCE_STIMULUS_H

#include "types.h"
#include "utils.h"
#include "entity.h"
#include "generator.h"
#include "neurons.h"

#define COND_E  m_parameters[0]

namespace dynclamp {

namespace generators {

class ConductanceStimulus : public Generator {
public:
        ConductanceStimulus(double Erev, uint id = GetId());
        virtual void initialise();
        virtual bool hasNext() const;
        virtual void step();
        virtual double output() const;
protected:
        virtual void addPost(Entity *entity);
private:
        double m_output;
        neurons::Neuron *m_neuron;
};

} // namespace dynclamp

} // namespace generators

/***
 *   FACTORY METHODS
 ***/
#ifdef __cplusplus
extern "C" {
#endif

dynclamp::Entity* ConductanceStimulusFactory(dictionary& args);
	
#ifdef __cplusplus
}
#endif

#endif

