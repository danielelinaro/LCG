#ifndef CONDUCTANCE_STIMULUS_H
#define CONDUCTANCE_STIMULUS_H

#include "types.h"
#include "utils.h"
#include "entity.h"
#include "generator.h"
#include "neurons.h"

#define COND_E  m_parameters["E"]
#define NMDA_COND_K1 m_parameters["K1"]
#define NMDA_COND_K2 m_parameters["K2"]

namespace lcg {

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
protected:
        double m_output;
        neurons::Neuron *m_neuron;
};

class NMDAConductanceStimulus : public ConductanceStimulus {
public:
        NMDAConductanceStimulus(double E, double K1, double K2, uint id = GetId());
        virtual void step();
};

} // namespace lcg

} // namespace generators

/***
 *   FACTORY METHODS
 ***/
#ifdef __cplusplus
extern "C" {
#endif

lcg::Entity* ConductanceStimulusFactory(string_dict& args);
lcg::Entity* NMDAConductanceStimulusFactory(string_dict& args);
	
#ifdef __cplusplus
}
#endif

#endif

