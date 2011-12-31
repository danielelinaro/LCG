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
        virtual double getOutput() const;
        virtual void step() = 0;
        
protected:
        double m_tPrevSpike;
};

//~~

class ExponentialSynapse : public Synapse {
public:
	ExponentialSynapse(double E, double dg, double tau, uint id = GetId(), double dt = GetGlobalDt());
	virtual void step();	
	virtual void handleEvent(const Event *event);
};

//~~

class Exp2Synapse : public Synapse {
public:
	Exp2Synapse(double E, double dg, double tau[2], uint id = GetId(), double dt = GetGlobalDt());
	virtual void step();
	virtual void handleEvent(const Event *event);
};


//~~

#define TAU_FACIL               m_parameters[3]
#define ONE_OVER_TAU_1		m_parameters[5]
#define ONE_OVER_TAU_REC	m_parameters[6]
#define ONE_OVER_TAU_FACIL	m_parameters[7]

class TsodyksSynapse : public Synapse {
public:
	TsodyksSynapse(double E, double dg, double U, double tau[3], uint id = GetId(), double dt = GetGlobalDt());
	virtual void step();
	virtual void handleEvent(const Event *event);
private:
	double m_t;
};

} // namespace synapses

} // namespace dynclamp

#endif
