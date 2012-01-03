#include "ou.h"
#include <iostream>

namespace dynclamp {

OU::OU(double sigma, double tau, double eta0, double seed, uint id, double dt)
        : DynamicalEntity(id, dt), m_randn(0, 1, seed)
{
        m_parameters.push_back(sigma);  // m_parameters[0]
        m_parameters.push_back(tau);    // m_parameters[1]
        m_parameters.push_back(eta0);   // m_parameters[2]

        m_mu = exp(-m_dt/TAU);
        m_coeff = sqrt(SIGMA*SIGMA * (1-m_mu*m_mu));
        m_state.push_back(eta0);
} 

void OU::evolve()
{
        m_noise = m_coeff * m_randn.random();
        ETA = ETA0 + m_mu*(ETA-ETA0) + m_noise;
}

OUcurrent::OUcurrent(double sigma, double tau, double I0, double seed, uint id, double dt)
        : OU(sigma, tau, I0, seed, id, dt)
{}

double OUcurrent::output() const
{
        // ETA is a current
        return ETA;
}

OUconductance::OUconductance(double sigma, double tau, double E, double G0, double seed, uint id, double dt)
        : OU(sigma, tau, G0, seed, id, dt)
{
        m_parameters.push_back(E);      // m_parameters[3]
}

double OUconductance::output() const
{
        // ETA is a conductance
        return ETA * (m_parameters[3] - m_post[0]->output());
}

} // namespace dynclamp

