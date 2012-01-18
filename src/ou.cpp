#include "ou.h"
#include <iostream>

dynclamp::Entity* OUcurrentFactory(dictionary& args)
{
        uint id;
        ullong seed;
        double sigma, tau, I0, dt;
        dynclamp::GetIdAndDtFromDictionary(args, &id, &dt);
        dynclamp::GetSeedFromDictionary(args, &seed);
        if ( ! dynclamp::CheckAndExtractDouble(args, "sigma", &sigma) ||
             ! dynclamp::CheckAndExtractDouble(args, "tau", &tau) ||
             ! dynclamp::CheckAndExtractDouble(args, "I0", &I0))
                return NULL;
        return new dynclamp::OUcurrent(sigma, tau, I0, seed, id, dt);
}

dynclamp::Entity* OUconductanceFactory(dictionary& args)
{
        uint id;
        double sigma, tau, E, G0, seed, dt;
        if ( ! dynclamp::CheckAndExtractDouble(args, "sigma", &sigma) ||
             ! dynclamp::CheckAndExtractDouble(args, "tau", &tau) ||
             ! dynclamp::CheckAndExtractDouble(args, "E", &E) ||
             ! dynclamp::CheckAndExtractDouble(args, "G0", &G0))
                return NULL;
        return new dynclamp::OUconductance(sigma, tau, E, G0, seed, id, dt);
}

namespace dynclamp {

OU::OU(double sigma, double tau, double eta0, ullong seed, uint id, double dt)
        : DynamicalEntity(id, dt), m_randn(0, 1, seed)
{
        m_parameters.push_back(sigma);  // m_parameters[0] -> sigma
        m_parameters.push_back(tau);    // m_parameters[1] -> tau
        m_parameters.push_back(eta0);   // m_parameters[2] -> eta0
        m_parameters.push_back(exp(-m_dt/OU_TAU)); // m_parameters[3] -> mu
        m_parameters.push_back(sqrt(OU_SIGMA*OU_SIGMA * (1-OU_MU*OU_MU)));        // m_parameters[4] -> coefficient

        m_state.push_back(eta0);
} 

void OU::evolve()
{
        OU_ETA = OU_ETA0 + OU_MU*(OU_ETA-OU_ETA0) + OU_COEFF * m_randn.random();
}

OUcurrent::OUcurrent(double sigma, double tau, double I0, ullong seed, uint id, double dt)
        : OU(sigma, tau, I0, seed, id, dt)
{}

double OUcurrent::output() const
{
        // OU_ETA is a current
        return OU_ETA;
}

OUconductance::OUconductance(double sigma, double tau, double E, double G0, ullong seed, uint id, double dt)
        : OU(sigma, tau, G0, seed, id, dt)
{
        m_parameters.push_back(E);      // m_parameters[5] -> reversal potential
}

double OUconductance::output() const
{
        // OU_ETA is a conductance
        return OU_ETA * (OU_E - m_post[0]->output());
}

} // namespace dynclamp

