#ifndef CONDUCTANCE_STIMULUS_H
#define CONDUCTANCE_STIMULUS_H

#include "types.h"
#include "utils.h"
#include "entity.h"
#include "generator.h"
#include "neurons.h"

#define COND_E  m_parameters["E"]

namespace dynclamp {

namespace generators {

class ConductanceStimulus : public Generator {
public:
        ConductanceStimulus(double E, uint id = GetId());
        virtual bool initialise();
        virtual bool hasNext() const;
        virtual void step();
        virtual double output();
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

dynclamp::Entity* ConductanceStimulusFactory(string_dict& args);
	
#ifdef __cplusplus
}
#endif

#endif

