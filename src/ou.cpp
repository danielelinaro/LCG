#include "ou.h"
#include "engine.h"
#include "neurons.h"
#include <iostream>

dynclamp::Entity* OUcurrentFactory(dictionary& args)
{
        uint id;
        ullong seed;
        double sigma, tau, I0;
        id = dynclamp::GetIdFromDictionary(args);
        seed = dynclamp::GetSeedFromDictionary(args);
        if ( ! dynclamp::CheckAndExtractDouble(args, "sigma", &sigma) ||
             ! dynclamp::CheckAndExtractDouble(args, "tau", &tau) ||
             ! dynclamp::CheckAndExtractDouble(args, "I0", &I0))
                return NULL;
        return new dynclamp::OUcurrent(sigma, tau, I0, seed, id);
}

dynclamp::Entity* OUconductanceFactory(dictionary& args)
{
        uint id;
        ullong seed;
        double sigma, tau, E, G0;
        id = dynclamp::GetIdFromDictionary(args);
        seed = dynclamp::GetSeedFromDictionary(args);
        if ( ! dynclamp::CheckAndExtractDouble(args, "sigma", &sigma) ||
             ! dynclamp::CheckAndExtractDouble(args, "tau", &tau) ||
             ! dynclamp::CheckAndExtractDouble(args, "E", &E) ||
             ! dynclamp::CheckAndExtractDouble(args, "G0", &G0))
                return NULL;
        return new dynclamp::OUconductance(sigma, tau, E, G0, seed, id);
}

namespace dynclamp {

OU::OU(double sigma, double tau, double eta0, ullong seed, uint id)
        : DynamicalEntity(id), m_randn(0, 1, seed)
{
        // See the paper [Gillespie, 1994, PRE] for explanation of the meaning of parameters
        // and of the method of solution.
        m_parameters.push_back(sigma);  // m_parameters[0] -> sigma
        m_parameters.push_back(tau);    // m_parameters[1] -> tau
        m_parameters.push_back(eta0);   // m_parameters[2] -> eta0
        m_parameters.push_back(2*OU_SIGMA*OU_SIGMA/OU_TAU);                     // m_parameters[3] -> diffusion constant
        m_parameters.push_back(exp(-GetGlobalDt()/OU_TAU));                     // m_parameters[4] -> mu
        m_parameters.push_back(sqrt(OU_CONST*OU_TAU/2 * (1-OU_MU*OU_MU)));      // m_parameters[5] -> coefficient
        m_parameters.push_back((double) seed);

        m_state.push_back(eta0);
} 

void OU::initialise()
{
        OU_ETA = OU_ETA0;
}

void OU::evolve()
{
        OU_ETA = OU_ETA0*(1-OU_MU) + OU_MU*OU_ETA + OU_COEFF*m_randn.random();
}

OUcurrent::OUcurrent(double sigma, double tau, double I0, ullong seed, uint id)
        : OU(sigma, tau, I0, seed, id)
{}

double OUcurrent::output() const
{
        // OU_ETA is a current
        return OU_ETA;
}

OUconductance::OUconductance(double sigma, double tau, double E, double G0, ullong seed, uint id)
        : OU(sigma, tau, G0, seed, id), m_neuron(NULL)
{
        m_parameters.push_back(E);      // m_parameters[6] -> reversal potential
}

double OUconductance::output() const
{
        // OU_ETA is a conductance
        return OU_ETA * (OU_E - m_neuron->output());
}

void OUconductance::addPost(Entity *entity)
{
        Logger(Debug, "OUconductance::addPost(Entity*)\n");
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

} // namespace dynclamp

