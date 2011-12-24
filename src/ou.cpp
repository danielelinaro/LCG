#include "ou.h"
#include <iostream>

namespace dynclamp {

OU::OU(double sigma, double tau, double eta0, double seed, uint id, double dt)
        : DynamicalEntity(id, dt), m_randn(0, 1, seed)
{
        m_parameters.push_back(sigma);
        m_parameters.push_back(tau);
        m_parameters.push_back(eta0);

        m_mu = exp(-m_dt/TAU);
        m_coeff = sqrt(SIGMA*SIGMA * (1-m_mu*m_mu));
        m_state.push_back(eta0);
} 

void OU::step()
{
        m_noise = m_coeff * m_randn.random();
        ETA = ETA0 + m_mu*ETA + m_noise;
}

double OU::getOutput() const
{
        return ETA;
}

} // namespace dynclamp

