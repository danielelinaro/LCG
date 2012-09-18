#ifndef OU_H
#define OU_H

#include "types.h"
#include "dynamical_entity.h"
#include "randlib.h"

#define OU_ETA       m_state[0]
#define OU_ETA_AUX   m_state[1]

#define OU_SIGMA m_parameters["sigma"]
#define OU_TAU   m_parameters["tau"]
#define OU_ETA0  m_parameters["eta0"]
#define OU_CONST m_parameters["const"]
#define OU_MU    m_parameters["mu"]
#define OU_COEFF m_parameters["coeff"]
#define OU_SEED  m_parameters["seed"]
#define OU_START m_parameters["start"]
#define OU_STOP  m_parameters["stop"]

#define OU_E     m_parameters["E"]

namespace dynclamp {

namespace neurons {
class Neuron;
}

double largeInterval[2] = {0, 365*24*60*60};

class OU : public DynamicalEntity
{
public:
        OU(double sigma, double tau, double eta0, ullong seed,
           double *interval = largeInterval, uint id = GetId());
        virtual bool initialise();
protected:
        virtual void evolve();
private:
        double m_mu, m_noise, m_coeff;
        NormalRandom m_randn;
};

class OUcurrent : public OU
{
public:
        OUcurrent(double sigma, double tau, double I0, ullong seed,
                  double *interval = largeInterval, uint id = GetId());
        virtual double output();
};

class OUconductance : public OU
{
public:
        OUconductance(double sigma, double tau, double E, double G0, ullong seed,
                      double *interval = largeInterval, uint id = GetId());
        virtual double output();
protected:
        virtual void addPost(Entity *entity);
private:
        neurons::Neuron *m_neuron;
};

}

/***
 *   FACTORY METHODS
 ***/
#ifdef __cplusplus
extern "C" {
#endif

dynclamp::Entity* OUcurrentFactory(string_dict& args);
dynclamp::Entity* OUconductanceFactory(string_dict& args);
	
#ifdef __cplusplus
}
#endif

#endif

