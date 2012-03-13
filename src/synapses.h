#ifndef SYNAPSES_H
#define SYNAPSES_H

#include <queue>
#include <math.h>
#include <stdio.h>
#include "dynamical_entity.h"

#define SYN_G m_state[0]
#define SYN_E m_parameters[0]
#define SYN_W m_parameters[1]
#define SYN_DELAY m_parameters[2]

namespace dynclamp {

namespace neurons {
class Neuron;
} // namespace neurons

namespace synapses {

class Synapse : public DynamicalEntity {
public:
	Synapse(double E, double weight, double delay, uint id = GetId());
        virtual void initialise();
	
        void setWeight(double weight);
        double g() const;
        virtual double output() const;
        
        virtual void handleEvent(const Event *event);

protected:
        bool processSpikes();
        virtual void handleSpike() = 0;
        virtual void addPost(Entity *entity);

protected:
        std::deque<double> m_spikeTimeouts;
        double m_tPrevSpike;

private:
        neurons::Neuron *m_neuron;
};

//~~

#define EXP_SYN_DECAY m_parameters[3]

class ExponentialSynapse : public Synapse {
public:
	ExponentialSynapse(double E, double weight, double delay, double tau, uint id = GetId());
protected:
	virtual void handleSpike();
	virtual void evolve();	
};

//~~

#define EXP2_SYN_DECAY1 m_parameters[3]
#define EXP2_SYN_DECAY2 m_parameters[4]
#define EXP2_SYN_FACTOR m_parameters[5]

class Exp2Synapse : public Synapse {
public:
	Exp2Synapse(double E, double weight, double delay, double tau[2], uint id = GetId());
        virtual void initialise();
protected:
	virtual void handleSpike();
	virtual void evolve();	
};


//~~

#define TMG_SYN_U                       m_parameters[3]
#define TMG_SYN_TAU_FACIL               m_parameters[4]
#define TMG_SYN_DECAY                   m_parameters[5]
#define TMG_SYN_ONE_OVER_TAU_1		m_parameters[6]
#define TMG_SYN_ONE_OVER_TAU_REC	m_parameters[7]
#define TMG_SYN_ONE_OVER_TAU_FACIL	m_parameters[8]
#define TMG_SYN_COEFF                   m_parameters[9]

class TMGSynapse : public Synapse {
public:
	TMGSynapse(double E, double weight, double delay, double U, double tau[3], uint id = GetId());
        virtual void initialise();
protected:
	virtual void handleSpike();
	virtual void evolve();	
};

} // namespace synapses

} // namespace dynclamp

/***
 *   FACTORY METHODS
 ***/
#ifdef __cplusplus
extern "C" {
#endif

dynclamp::Entity* ExponentialSynapseFactory(dictionary& args);
dynclamp::Entity* Exp2SynapseFactory(dictionary& args);
dynclamp::Entity* TMGSynapseFactory(dictionary& args);
	
#ifdef __cplusplus
}
#endif


#endif
