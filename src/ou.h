#ifndef OU_H
#define OU_H

#include "types.h"
#include "dynamical_entity.h"
#include "randlib.h"

#define ETA   m_state[0]

#define SIGMA m_parameters[0]
#define TAU   m_parameters[1]
#define ETA0  m_parameters[2]

namespace dynclamp {

class OU : public DynamicalEntity
{
public:
        OU(double sigma, double tau, double eta0 = 0.0, double seed = SEED,
           uint id = GetId(), double dt = GetGlobalDt());
protected:
        virtual void evolve();
private:
        double m_mu, m_noise, m_coeff;
        NormalRandomBM m_randn;
};

class OUcurrent : public OU
{
public:
        OUcurrent(double sigma, double tau, double I0 = 0.0, double seed = SEED,
                  uint id = GetId(), double dt = GetGlobalDt());
        virtual double output();
};

class OUconductance : public OU
{
public:
        OUconductance(double sigma, double tau, double E, double G0 = 0.0, double seed = SEED,
                      uint id = GetId(), double dt = GetGlobalDt());
        virtual double output();
};

}

#endif

