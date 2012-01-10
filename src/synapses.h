#ifndef SYNAPSES_H
#define SYNAPSES_H

#include <cmath>
#include <cstdio>
#include "dynamical_entity.h"

#define G_SYN m_state[0]
#define E_SYN m_parameters[0]

namespace dynclamp {

namespace synapses {

class Synapse : public DynamicalEntity {
public:
	Synapse(double E, uint id = GetId(), double dt = GetGlobalDt());
	
        double g() const;
        virtual double output() const;
        
protected:
        //virtual void finalizeConnect(DynamicalEntity *entity);

protected:
        double m_tPrevSpike;

private:
        DynamicalEntity *m_postSynapticNeuron;
};

//~~

class ExponentialSynapse : public Synapse {
public:
	ExponentialSynapse(double E, double weight, double tau, uint id = GetId(), double dt = GetGlobalDt());
	virtual void handleEvent(const Event *event);
protected:
	virtual void evolve();	
};

//~~

class Exp2Synapse : public Synapse {
public:
	Exp2Synapse(double E, double weight, double tau[2], uint id = GetId(), double dt = GetGlobalDt());
	virtual void handleEvent(const Event *event);
protected:
	virtual void evolve();	
};


//~~

#define TAU_FACIL               m_parameters[3]
#define ONE_OVER_TAU_1		m_parameters[5]
#define ONE_OVER_TAU_REC	m_parameters[6]
#define ONE_OVER_TAU_FACIL	m_parameters[7]

class TMGSynapse : public Synapse {
public:
	TMGSynapse(double E, double weight, double U, double tau[3], uint id = GetId(), double dt = GetGlobalDt());
	virtual void handleEvent(const Event *event);
protected:
	virtual void evolve();	
private:
	double m_t;
};

} // namespace synapses

} // namespace dynclamp

#endif
