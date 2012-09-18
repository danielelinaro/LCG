#include <stdlib.h>
#include "synapses.h"
#include "engine.h"
#include "neurons.h"

dynclamp::Entity* ExponentialSynapseFactory(string_dict& args)
{
        uint id;
        double E, weight, tau;
        id = dynclamp::GetIdFromDictionary(args);
        if ( ! dynclamp::CheckAndExtractDouble(args, "E", &E) ||
             ! dynclamp::CheckAndExtractDouble(args, "weight", &weight) ||
             ! dynclamp::CheckAndExtractDouble(args, "tau", &tau)) {
                dynclamp::Logger(dynclamp::Critical, "Unable to build an exponential synapse.\n");
                return NULL;
        }
        return new dynclamp::synapses::ExponentialSynapse(E, weight, tau, id);
}

dynclamp::Entity* Exp2SynapseFactory(string_dict& args)
{        
        uint id;
        double E, weight, tau[2];
        id = dynclamp::GetIdFromDictionary(args);
        if ( ! dynclamp::CheckAndExtractDouble(args, "E", &E) ||
             ! dynclamp::CheckAndExtractDouble(args, "weight", &weight) ||
             ! dynclamp::CheckAndExtractDouble(args, "tauRise", &tau[0]) ||
             ! dynclamp::CheckAndExtractDouble(args, "tauDecay", &tau[1])) {
                dynclamp::Logger(dynclamp::Critical, "Unable to build a biexponential synapse.\n");
                return NULL;
        }
        return new dynclamp::synapses::Exp2Synapse(E, weight, tau, id);
}

dynclamp::Entity* TMGSynapseFactory(string_dict& args)
{
        uint id;
        double E, weight, U, tau[3];
        id = dynclamp::GetIdFromDictionary(args);
        if ( ! dynclamp::CheckAndExtractDouble(args, "E", &E) ||
             ! dynclamp::CheckAndExtractDouble(args, "weight", &weight) ||
             ! dynclamp::CheckAndExtractDouble(args, "U", &U) ||
             ! dynclamp::CheckAndExtractDouble(args, "tau1", &tau[0]) ||
             ! dynclamp::CheckAndExtractDouble(args, "tauRec", &tau[1]) ||
             ! dynclamp::CheckAndExtractDouble(args, "tauFacil", &tau[2])) {
                dynclamp::Logger(dynclamp::Critical, "Unable to build a Tsodyks-Markram synapse.\n");
                return NULL;
        }
        return new dynclamp::synapses::TMGSynapse(E, weight, U, tau, id);
}

namespace dynclamp {

namespace synapses {

Synapse::Synapse(double E, double weight, uint id)
        : DynamicalEntity(id), m_neuron(NULL), m_spikeTimeouts()
{
        m_state.push_back(0.0);         // m_state[0] -> conductance
        SYN_E = E;
        SYN_W = weight;
        setName("Synapse");
        setUnits("pA");
}

bool Synapse::initialise()
{
        m_tPrevSpike = -1000.0;
        SYN_G = 0.0;
        return true;
}

double Synapse::output()
{
        // i = g * (E - V_post)
        return SYN_G * (SYN_E - m_neuron->output());
}

double Synapse::g() const
{
	return SYN_G;
}

void Synapse::handleEvent(const Event *event)
{
        if (event->type() == SPIKE)
                handleSpike();
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

ExponentialSynapse::ExponentialSynapse(double E, double weight, double tau,
                                       uint id)
        : Synapse(E, weight, id)
{
        EXP_SYN_DECAY = exp(-GetGlobalDt()/tau);
        setName("ExponentialSynapse");
        setUnits("pA");
}

bool ExponentialSynapse::initialise()
{
        return Synapse::initialise();
}

void ExponentialSynapse::evolve()
{
	SYN_G = SYN_G * EXP_SYN_DECAY;
}

void ExponentialSynapse::handleSpike()
{
	SYN_G += SYN_W;
}

//~~~

Exp2Synapse::Exp2Synapse(double E, double weight, double tau[2],
                         uint id) :
	Synapse(E, weight, id)
{
        m_state.push_back(0.0);         // m_state[1]
        m_state.push_back(0.0);         // m_state[2]

        double dt = GetGlobalDt();
	double tp = (tau[0]*tau[1])/(tau[1] - tau[0]) * log(tau[1]/tau[0]);
        EXP2_SYN_DECAY1 = exp(-dt/tau[0]);
        EXP2_SYN_DECAY2 = exp(-dt/tau[1]);
	EXP2_SYN_FACTOR = 1. / (-exp(-tp/tau[0]) + exp(-tp/tau[1]));
        setName("Exp2Synapse");
        setUnits("pA");
}

bool Exp2Synapse::initialise()
{
        if (! Synapse::initialise())
                return false;
        m_state[1] = 0.0;
        m_state[2] = 0.0;
        return true;
}

void Exp2Synapse::evolve()
{
        m_state[1] = m_state[1] * EXP2_SYN_DECAY1;
        m_state[2] = m_state[2] * EXP2_SYN_DECAY2;
        SYN_G = m_state[2] - m_state[1];
}
	
void Exp2Synapse::handleSpike()
{
        Logger(Debug, "Exp2Synapse::handleSpike()\n");
        m_state[1] += SYN_W * EXP2_SYN_FACTOR;
        m_state[2] += SYN_W * EXP2_SYN_FACTOR;
}

//~~~

TMGSynapse::TMGSynapse(double E, double weight, double U, double tau[3],
                       uint id)
        : Synapse(E, weight, id)
{
        // tau = {tau_1, tau_rec, tau_facil}
        m_state.push_back(0.0);         // m_state[1] -> y
        m_state.push_back(0.0);         // m_state[2] -> z
        m_state.push_back(U);           // m_state[3] -> u

        TMG_SYN_U = U;
        TMG_SYN_TAU_1 = tau[0];
        TMG_SYN_TAU_REC = tau[1];
        TMG_SYN_TAU_FACIL = tau[2];
        TMG_SYN_DECAY = exp(-GetGlobalDt()/TMG_SYN_TAU_1);
        TMG_SYN_ONE_OVER_TAU_1 = 1.0 / TMG_SYN_TAU_1;
        TMG_SYN_ONE_OVER_TAU_REC = 1.0 / TMG_SYN_TAU_REC;
        TMG_SYN_ONE_OVER_TAU_FACIL = 1.0 / TMG_SYN_TAU_FACIL;
	TMG_SYN_COEFF = 1.0 / ((tau[0]/tau[1])-1.);

        setName("TMGSynapse");
        setUnits("pA");
}

bool TMGSynapse::initialise()
{
        if (! Synapse::initialise())
                return false;
        m_state[1] = 0.0;
        m_state[2] = 0.0;
        m_state[3] = TMG_SYN_U;
        return true;
}

void TMGSynapse::evolve()
{
	SYN_G = SYN_G * TMG_SYN_DECAY;
}
	
void TMGSynapse::handleSpike()
{
        double now = GetGlobalTime();
        // first calculate z at event, based on prior y and z
        m_state[2] = m_state[2] * exp(-(now - m_tPrevSpike) * TMG_SYN_ONE_OVER_TAU_REC);
        m_state[2] += (m_state[1] * (exp(-(now - m_tPrevSpike) * TMG_SYN_ONE_OVER_TAU_1) -
        	       exp(-(now - m_tPrevSpike) * TMG_SYN_ONE_OVER_TAU_REC)) * TMG_SYN_COEFF);

        // then calculate y at event
        m_state[1] = m_state[1] * exp(-(now - m_tPrevSpike) * TMG_SYN_ONE_OVER_TAU_1);
        double x = 1. - m_state[1] - m_state[2];
        	
        // calculate u at event
        if (TMG_SYN_TAU_FACIL > 0.) {
        	m_state[3] = m_state[3] * exp(-(now - m_tPrevSpike) * TMG_SYN_ONE_OVER_TAU_FACIL);
        	m_state[3] += TMG_SYN_U * (1. - m_state[3]);
        }
        else {
        	m_state[3] = TMG_SYN_U;
        }
        	
        double xu = x * m_state[3];
        SYN_G += SYN_W * xu;
        m_state[1] += xu;
        
        m_tPrevSpike = now;
}

} // namespace synapses

} // namespace dynclamp

