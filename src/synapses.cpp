#include "synapses.h"
#include <cstdlib>

namespace dynclamp {

namespace synapses {

Synapse::Synapse(double E, uint id, double dt)
        : DynamicalEntity(id, dt), m_tPrevSpike(-1000.0), m_postSynapticNeuron(NULL)
{
        m_state.push_back(0.0);         // m_state[0] -> conductance
        m_parameters.push_back(E);      // m_parameters[0] -> reversal potential
}

double Synapse::output() const
{
        // i = g * (E - V_post)
        //return G_SYN * (m_postSynapticNeuron->output() - E_SYN);
        return G_SYN * (E_SYN - m_post[0]->output());
}

/*
void Synapse::finalizeConnect(DynamicalEntity *entity)
{
        Logger(Debug, "Synapse::finalizeConnect(DynamicalEntity *)\n");
        m_postSynapticNeuron = entity;
}
*/

double Synapse::g() const
{
	return G_SYN;
}

//~~~

ExponentialSynapse::ExponentialSynapse(double E, double dg, double tau,
                                       uint id, double dt)
        : Synapse(E, id, dt)
{
        m_parameters.push_back(dg);             // m_parameters[1]
        m_parameters.push_back(exp(-dt/tau));   // m_parameters[2]
}

void ExponentialSynapse::evolve()
{
	G_SYN = G_SYN * m_parameters[2];
}

void ExponentialSynapse::handleEvent(const Event *event)
{
        if (event->type() == SPIKE && event->hasExpired())
	        G_SYN += m_parameters[1];
}

//~~~

Exp2Synapse::Exp2Synapse(double E, double dg, double tau[2],
                         uint id, double dt) :
	Synapse(E, id, dt)
{
        m_state.push_back(0.0);         // m_state[1]
        m_state.push_back(0.0);         // m_state[2]

	double tp = (tau[0]*tau[1])/(tau[1] - tau[0]) * log(tau[1]/tau[0]);
        m_parameters.push_back(dg);                     // m_parameters[1]
        m_parameters.push_back(exp(-dt/tau[0]));        // m_parameters[2]
        m_parameters.push_back(exp(-dt/tau[1]));        // m_parameters[3]
	m_parameters.push_back(1. / (-exp(-tp/tau[0]) + exp(-tp/tau[1])));  // m_parameters[4]
}

void Exp2Synapse::evolve()
{
	m_state[1] = m_state[1] * m_parameters[2];
	m_state[2] = m_state[2] * m_parameters[3];
	G_SYN = m_state[2] - m_state[1];
}
	
void Exp2Synapse::handleEvent(const Event *event)
{
        if (event->type() == SPIKE && event->hasExpired()) {
        	m_state[1] += m_parameters[1] * m_parameters[4];
        	m_state[2] += m_parameters[1] * m_parameters[4];
        }
}

//~~~

TsodyksSynapse::TsodyksSynapse(double E, double dg, double U, double tau[3],
                               uint id, double dt) : Synapse(E, id, dt), m_t(0.0)
{
        // tau = {tau_1, tau_rec, tau_facil}
        m_state.push_back(0.0);         // m_state[1] -> y
        m_state.push_back(0.0);         // m_state[2] -> z
        m_state.push_back(U);           // m_state[3] -> u

        m_parameters.push_back(dg);     // m_parameters[1] -> dg
        m_parameters.push_back(U);      // m_parameters[2] -> U
        m_parameters.push_back(tau[2]); // m_parameters[3] -> tau_facil
        m_parameters.push_back(exp(-m_dt/tau[0]));      // m_parameters[4] -> exp. decay
        m_parameters.push_back(1.0 / tau[0]);           // m_parameters[5] -> 1 / tau_1
        m_parameters.push_back(1.0 / tau[1]);           // m_parameters[6] -> 1 / tau_rec
        m_parameters.push_back(1.0 / tau[2]);           // m_parameters[7] -> 1 / tau_facil
	m_parameters.push_back(1.0 / ((tau[0]/tau[1])-1.));    // m_parameters[8] -> coeff.
}

void TsodyksSynapse::evolve()
{
        m_t += m_dt;
	G_SYN = G_SYN * m_parameters[4];
}
	
void TsodyksSynapse::handleEvent(const Event *event)
{
        if (event->type() == SPIKE && event->hasExpired()) {
        	// first calculate z at event, based on prior y and z
                m_state[2] = m_state[2] * exp(-(m_t - m_tPrevSpike) * ONE_OVER_TAU_REC);
        	m_state[2] += (m_state[1] * (exp(-(m_t - m_tPrevSpike) * ONE_OVER_TAU_1) -
        			exp(-(m_t - m_tPrevSpike) * ONE_OVER_TAU_REC)) * m_parameters[8]);
        
        	// then calculate y at event
        	m_state[1] = m_state[1] * exp(-(m_t - m_tPrevSpike) * ONE_OVER_TAU_1);
        	double x = 1. - m_state[1] - m_state[2];
        		
        	// calculate u at event
        	if (TAU_FACIL > 0.) {
        		m_state[3] = m_state[3] * exp(-(m_t - m_tPrevSpike) * ONE_OVER_TAU_FACIL);
        		m_state[3] += m_parameters[2] * (1. - m_state[3]);
        	}
        	else {
        		m_state[3] = m_parameters[2];
        	}
        		
                double xu = x * m_state[3];
        	G_SYN += m_parameters[1] * xu;
        	m_state[1] += xu;

                m_tPrevSpike = m_t;
        }
}

} // namespace synapses

} // namespace dynclamp

