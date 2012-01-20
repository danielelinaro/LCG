#ifndef OU_H
#define OU_H

#include "types.h"
#include "dynamical_entity.h"
#include "randlib.h"

#define OU_ETA   m_state[0]

#define OU_SIGMA m_parameters[0]
#define OU_TAU   m_parameters[1]
#define OU_ETA0  m_parameters[2]
#define OU_CONST m_parameters[3]
#define OU_MU    m_parameters[4]
#define OU_COEFF m_parameters[5]

#define OU_E     m_parameters[6]

namespace dynclamp {

class OU : public DynamicalEntity
{
public:
        OU(double sigma, double tau, double eta0, ullong seed = SEED,
           uint id = GetId(), double dt = GetGlobalDt());
protected:
        virtual void evolve();
private:
        double m_mu, m_noise, m_coeff;
        NormalRandom m_randn;
};

class OUcurrent : public OU
{
public:
        OUcurrent(double sigma, double tau, double I0, ullong seed = SEED,
                  uint id = GetId(), double dt = GetGlobalDt());
        virtual double output() const;
};

class OUconductance : public OU
{
public:
        OUconductance(double sigma, double tau, double E, double G0, ullong seed = SEED,
                      uint id = GetId(), double dt = GetGlobalDt());
        virtual double output() const;
};

}

/***
 *   FACTORY METHODS
 ***/
#ifdef __cplusplus
extern "C" {
#endif

dynclamp::Entity* OUcurrentFactory(dictionary& args);
dynclamp::Entity* OUconductanceFactory(dictionary& args);
	
#ifdef __cplusplus
}
#endif

#endif

