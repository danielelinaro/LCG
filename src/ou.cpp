#include "ou.h"
#include "engine.h"
#include "neurons.h"
#include <iostream>
#include <string>
#include <sstream>

dynclamp::Entity* OUcurrentFactory(dictionary& args)
{
        uint id;
        ullong seed;
        double sigma, tau, I0, interval[2];
        std::string intervalStr;
        id = dynclamp::GetIdFromDictionary(args);
        seed = dynclamp::GetSeedFromDictionary(args);
        if ( ! dynclamp::CheckAndExtractDouble(args, "sigma", &sigma) ||
             ! dynclamp::CheckAndExtractDouble(args, "tau", &tau) ||
             ! dynclamp::CheckAndExtractDouble(args, "I0", &I0)) {
                dynclamp::Logger(dynclamp::Critical, "Unable to build a OUcurrent.\n");
                return NULL;
        }
        if (dynclamp::CheckAndExtractValue(args, "interval", intervalStr)) {
                size_t stop = intervalStr.find(",",0);
                if (stop == intervalStr.npos) {
                        dynclamp::Logger(dynclamp::Critical, "Error in the definition of the interval"
                                        "(which should be composed of two comma-separated values.\n");
                        return NULL;
                }
                {
                        std::stringstream ss(intervalStr.substr(0,stop));
                        ss >> interval[0];
                }
                {
                        std::stringstream ss(intervalStr.substr(stop+1));
                        ss >> interval[1];
                }
                dynclamp::Logger(dynclamp::Debug, "Interval = [%g,%g].\n", interval[0], interval[1]);
        }
        else {
                interval[0] = dynclamp::largeInterval[0];
                interval[1] = dynclamp::largeInterval[1];
        }
        return new dynclamp::OUcurrent(sigma, tau, I0, seed, interval, id);
}

dynclamp::Entity* OUconductanceFactory(dictionary& args)
{
        uint id;
        ullong seed;
        double sigma, tau, E, G0, interval[2];
        std::string intervalStr;
        id = dynclamp::GetIdFromDictionary(args);
        seed = dynclamp::GetSeedFromDictionary(args);
        if ( ! dynclamp::CheckAndExtractDouble(args, "sigma", &sigma) ||
             ! dynclamp::CheckAndExtractDouble(args, "tau", &tau) ||
             ! dynclamp::CheckAndExtractDouble(args, "E", &E) ||
             ! dynclamp::CheckAndExtractDouble(args, "G0", &G0)) {
                dynclamp::Logger(dynclamp::Critical, "Unable to build a OUconductance.\n");
                return NULL;
        }
        dynclamp::Logger(dynclamp::Debug, "G0 = %g sigma = %g tau = %g E = %g\n", G0, sigma, tau, E);
        if (dynclamp::CheckAndExtractValue(args, "interval", intervalStr)) {
                size_t stop = intervalStr.find(",",0);
                if (stop == intervalStr.npos) {
                        dynclamp::Logger(dynclamp::Critical, "Error in the definition of the interval"
                                        "(which should be composed of two comma-separated values.\n");
                        return NULL;
                }
                std::stringstream ss0(intervalStr.substr(0,stop)), ss1(intervalStr.substr(stop+1));
                ss0 >> interval[0];
                ss1 >> interval[1];
                dynclamp::Logger(dynclamp::Debug, "Interval = [%g,%g].\n", interval[0], interval[1]);
        }
        else {
                interval[0] = dynclamp::largeInterval[0];
                interval[1] = dynclamp::largeInterval[1];
        }
        return new dynclamp::OUconductance(sigma, tau, E, G0, seed, interval, id);
}

namespace dynclamp {

OU::OU(double sigma, double tau, double eta0, ullong seed, double interval[2], uint id)
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
        m_parameters.push_back((double) seed);  // m_parameters[6] -> seed
        m_parameters.push_back(interval[0]);    // m_parameters[7] -> start time
        m_parameters.push_back(interval[1]);    // m_parameters[8] -> end time

        m_state.push_back(0.0);         // m_state[0] -> eta
        m_state.push_back(0.0);         // m_state[1] -> auxiliary variable
        setName("OU");
} 

bool OU::initialise()
{
        OU_ETA = 0.0;
        OU_ETA_AUX = OU_ETA0;
        return true;
}

void OU::evolve()
{
        double now = GetGlobalTime();
        OU_ETA_AUX = OU_ETA0*(1-OU_MU) + OU_MU*OU_ETA_AUX + OU_COEFF*m_randn.random();
        if (now >= OU_START && now <= OU_STOP) {
                OU_ETA = OU_ETA_AUX;
        }
        else {
                OU_ETA = 0.0;
        }
}

OUcurrent::OUcurrent(double sigma, double tau, double I0, ullong seed, double interval[2], uint id)
        : OU(sigma, tau, I0, seed, interval, id)
{
        setName("OUcurrent");
        setUnits("pA");
}

double OUcurrent::output() const
{
        // OU_ETA is a current
        return OU_ETA;
}

OUconductance::OUconductance(double sigma, double tau, double E, double G0, ullong seed, double interval[2], uint id)
        : OU(sigma, tau, G0, seed, interval, id), m_neuron(NULL)
{
        m_parameters.push_back(E);      // m_parameters[9] -> reversal potential
        setName("OUconductance");
        setUnits("nS");
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

