#include "synapses.h"
#include "neurons.h"
#include <stdlib.h>

dynclamp::Entity* ExponentialSynapseFactory(dictionary& args)
{
        uint id;
        double E, weight, delay, tau;
        id = dynclamp::GetIdFromDictionary(args);
        if ( ! dynclamp::CheckAndExtractDouble(args, "E", &E) ||
             ! dynclamp::CheckAndExtractDouble(args, "weight", &weight) ||
             ! dynclamp::CheckAndExtractDouble(args, "delay", &delay) ||
             ! dynclamp::CheckAndExtractDouble(args, "tau", &tau))
                return NULL;
        return new dynclamp::synapses::ExponentialSynapse(E, weight, delay, tau, id);
}

dynclamp::Entity* Exp2SynapseFactory(dictionary& args)
{        
        uint id;
        double E, weight, delay, tau[2];
        id = dynclamp::GetIdFromDictionary(args);
        if ( ! dynclamp::CheckAndExtractDouble(args, "E", &E) ||
             ! dynclamp::CheckAndExtractDouble(args, "weight", &weight) ||
             ! dynclamp::CheckAndExtractDouble(args, "delay", &delay) ||
             ! dynclamp::CheckAndExtractDouble(args, "tauRise", &tau[0]) ||
             ! dynclamp::CheckAndExtractDouble(args, "tauDecay", &tau[1]))
                return NULL;
        return new dynclamp::synapses::Exp2Synapse(E, weight, delay, tau, id);
}

dynclamp::Entity* TMGSynapseFactory(dictionary& args)
{
        uint id;
        double E, weight, delay, U, tau[3];
        id = dynclamp::GetIdFromDictionary(args);
        if ( ! dynclamp::CheckAndExtractDouble(args, "E", &E) ||
             ! dynclamp::CheckAndExtractDouble(args, "weight", &weight) ||
             ! dynclamp::CheckAndExtractDouble(args, "delay", &delay) ||
             ! dynclamp::CheckAndExtractDouble(args, "U", &U) ||
             ! dynclamp::CheckAndExtractDouble(args, "tau1", &tau[0]) ||
             ! dynclamp::CheckAndExtractDouble(args, "tauRec", &tau[1]) ||
             ! dynclamp::CheckAndExtractDouble(args, "tauFacil", &tau[2]))
                return NULL;
        return new dynclamp::synapses::TMGSynapse(E, weight, delay, U, tau, id);
}

namespace dynclamp {

namespace synapses {

Synapse::Synapse(double E, double weight, double delay, uint id)
        : DynamicalEntity(id), m_tPrevSpike(-1000.0), m_neuron(NULL), m_spikeTimeouts()
{
        m_state.push_back(0.0);         // m_state[0] -> conductance
        m_parameters.push_back(E);      // m_parameters[0] -> reversal potential
        m_parameters.push_back(weight); // m_parameters[1] -> weight
        m_parameters.push_back(delay);  // m_parameters[2] -> delay
}

double Synapse::output() const
{
        // i = g * (E - V_post)
        return SYN_G * (SYN_E - m_neuron->output());
}

double Synapse::g() const
{
	return SYN_G;
}

void Synapse::setWeight(double weight)
{
        SYN_W = weight;
}

void Synapse::handleEvent(const Event *event)
{
        if (event->type() == SPIKE)
                m_spikeTimeouts.push_back(event->time() + SYN_DELAY);
}

bool Synapse::processSpikes()
{
        bool processedSpikes = false;
        double t = GetGlobalTime();
        while (!m_spikeTimeouts.empty() && m_spikeTimeouts.front() <= t) {
                m_spikeTimeouts.pop_front();
                handleSpike();
                processedSpikes = true;
        }
        return processedSpikes;
}

void Synapse::addPost(Entity *entity)
{
        Logger(Debug, "Synapse::addPost(Entity*)\n");
        Entity::addPost(entity);
        neurons::Neuron *n = dynamic_cast<neurons::Neuron*>(entity);
        if (n != NULL) {
                Logger(Debug, "Connected to a neuron (id #%d).\n", entity->id());
                m_neuron = n;
        }
        else {
                Logger(Debug, "Entity #%d is not a neuron.\n", entity->id());
        }
}

//~~~

ExponentialSynapse::ExponentialSynapse(double E, double weight, double delay, double tau,
                                       uint id)
        : Synapse(E, weight, delay, id)
{
        m_parameters.push_back(exp(-GetGlobalDt()/tau));   // m_parameters[3] -> decay coefficient
}

void ExponentialSynapse::evolve()
{
        if (! processSpikes())
	        SYN_G = SYN_G * EXP_SYN_DECAY;
}

void ExponentialSynapse::handleSpike()
{
	SYN_G += m_parameters[1];
}

//~~~

Exp2Synapse::Exp2Synapse(double E, double weight, double delay, double tau[2],
                         uint id) :
	Synapse(E, weight, delay, id)
{
        m_state.push_back(0.0);         // m_state[1]
        m_state.push_back(0.0);         // m_state[2]

        double dt = GetGlobalDt();
	double tp = (tau[0]*tau[1])/(tau[1] - tau[0]) * log(tau[1]/tau[0]);
        m_parameters.push_back(exp(-dt/tau[0]));        // m_parameters[3] -> first decay coefficient
        m_parameters.push_back(exp(-dt/tau[1]));        // m_parameters[4] -> second decay coefficient
	m_parameters.push_back(1. / (-exp(-tp/tau[0]) + exp(-tp/tau[1])));  // m_parameters[5] -> factor
}

void Exp2Synapse::evolve()
{
        if (! processSpikes()) {
        	m_state[1] = m_state[1] * EXP2_SYN_DECAY1;
        	m_state[2] = m_state[2] * EXP2_SYN_DECAY2;
        	SYN_G = m_state[2] - m_state[1];
        }
}
	
void Exp2Synapse::handleSpike()
{
        Logger(Debug, "Exp2Synapse::handleEvent(const Event*)\n");
        m_state[1] += SYN_W * EXP2_SYN_FACTOR;
        m_state[2] += SYN_W * EXP2_SYN_FACTOR;
}

//~~~

TMGSynapse::TMGSynapse(double E, double weight, double delay, double U, double tau[3],
                       uint id)
        : Synapse(E, weight, delay, id), m_t(0.0)
{
        // tau = {tau_1, tau_rec, tau_facil}
        m_state.push_back(0.0);         // m_state[1] -> y
        m_state.push_back(0.0);         // m_state[2] -> z
        m_state.push_back(U);           // m_state[3] -> u

        m_parameters.push_back(U);      // m_parameters[3] -> U
        m_parameters.push_back(tau[2]); // m_parameters[4] -> tau_facil
        m_parameters.push_back(exp(-GetGlobalDt()/tau[0]));      // m_parameters[5] -> exp. decay
        m_parameters.push_back(1.0 / tau[0]);           // m_parameters[6] -> 1 / tau_1
        m_parameters.push_back(1.0 / tau[1]);           // m_parameters[7] -> 1 / tau_rec
        m_parameters.push_back(1.0 / tau[2]);           // m_parameters[8] -> 1 / tau_facil
	m_parameters.push_back(1.0 / ((tau[0]/tau[1])-1.));    // m_parameters[9] -> coeff.
}

void TMGSynapse::evolve()
{
        if (! processSpikes()) {
                m_t += GetGlobalDt();
	        SYN_G = SYN_G * TMG_SYN_DECAY;
        }
}
	
void TMGSynapse::handleSpike()
{
        // first calculate z at event, based on prior y and z
        m_state[2] = m_state[2] * exp(-(m_t - m_tPrevSpike) * TMG_SYN_ONE_OVER_TAU_REC);
        m_state[2] += (m_state[1] * (exp(-(m_t - m_tPrevSpike) * TMG_SYN_ONE_OVER_TAU_1) -
        	       exp(-(m_t - m_tPrevSpike) * TMG_SYN_ONE_OVER_TAU_REC)) * TMG_SYN_COEFF);

        // then calculate y at event
        m_state[1] = m_state[1] * exp(-(m_t - m_tPrevSpike) * TMG_SYN_ONE_OVER_TAU_1);
        double x = 1. - m_state[1] - m_state[2];
        	
        // calculate u at event
        if (TMG_SYN_TAU_FACIL > 0.) {
        	m_state[3] = m_state[3] * exp(-(m_t - m_tPrevSpike) * TMG_SYN_ONE_OVER_TAU_FACIL);
        	m_state[3] += TMG_SYN_U * (1. - m_state[3]);
        }
        else {
        	m_state[3] = TMG_SYN_U;
        }
        	
        double xu = x * m_state[3];
        SYN_G += SYN_W * xu;
        m_state[1] += xu;
        
        m_tPrevSpike = m_t;
}

} // namespace synapses

} // namespace dynclamp

