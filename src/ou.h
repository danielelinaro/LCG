#ifndef OU_H
#define OU_H

#include "types.h"
#include "dynamical_entity.h"
#include "randlib.h"

#define OU_ETA      m_state[0]
#define OU_ETA_AUX  m_state[1]

#define OU_MEAN     m_parameters["mean"]
#define OU_STDDEV   m_parameters["stddev"]
#define OU_TAU      m_parameters["tau"]
#define OU_IC       m_parameters["ic"]
#define OU_CONST    m_parameters["const"]
#define OU_MU       m_parameters["mu"]
#define OU_COEFF    m_parameters["coeff"]
#define OU_SEED     m_parameters["seed"]
#define OU_START    m_parameters["start"]
#define OU_STOP     m_parameters["stop"]

#define OUNS_MEAN    m_inputs[0]
#define OUNS_STDDEV  m_inputs[1]

namespace lcg {

double largeInterval[2] = {0, 365*24*60*60};

class OU : public DynamicalEntity
{
public:
        OU(double mean, double stddev, double tau, double initialCondition, std::string units,
           ullong seed = 0, double *interval = largeInterval, uint id = GetId());
        virtual ~OU();
        virtual bool initialise();
        virtual double output();
protected:
        virtual void evolve();
private:
        double m_mu, m_noise, m_coeff;
        bool m_fixSeed;
        NormalRandom *m_randn;
};

class OUNonStationary : public DynamicalEntity
{
public:
        OUNonStationary(double tau, double initialCondition, std::string units,
           ullong seed = 0, double *interval = largeInterval, uint id = GetId());
        virtual ~OUNonStationary();
        virtual bool initialise();
        virtual double output();
protected:
        virtual void evolve();
private:
        double m_mu;
        bool m_fixSeed;
        NormalRandom *m_randn;
};


}

/***
 *   FACTORY METHODS
 ***/
#ifdef __cplusplus
extern "C" {
#endif

lcg::Entity* OUFactory(string_dict& args);
lcg::Entity* OUNonStationaryFactory(string_dict& args);
	
#ifdef __cplusplus
}
#endif

#endif

