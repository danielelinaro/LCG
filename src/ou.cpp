#include "ou.h"
#include "engine.h"
#include "neurons.h"
#include <iostream>
#include <string>
#include <sstream>

lcg::Entity* OUFactory(string_dict& args)
{
        uint id;
        ullong seed;
        double mean, stddev, tau, ic, interval[2];
        std::string units, intervalStr;
        id = lcg::GetIdFromDictionary(args);
        seed = lcg::GetSeedFromDictionary(args);
        if ( ! lcg::CheckAndExtractDouble(args, "mean", &mean) ||
             ! lcg::CheckAndExtractDouble(args, "stddev", &stddev) ||
             ! lcg::CheckAndExtractDouble(args, "tau", &tau) ||
             ! lcg::CheckAndExtractValue(args, "units", units) ||
             ! lcg::CheckAndExtractDouble(args, "initialCondition", &ic)) {
                lcg::Logger(lcg::Critical, "Unable to build a OU object.\n");
                return NULL;
        }
        if ( ! lcg::CheckAndExtractUnsignedLongLong(args, "seed", &seed))
                seed = 0;
        if (lcg::CheckAndExtractValue(args, "interval", intervalStr)) {
                size_t stop = intervalStr.find(",",0);
                if (stop == intervalStr.npos) {
                        lcg::Logger(lcg::Critical, "Error in the definition of the interval "
                                        "(which should be composed of two comma-separated values).\n");
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
                lcg::Logger(lcg::Debug, "Interval = [%g,%g].\n", interval[0], interval[1]);
        }
        else {
                interval[0] = lcg::largeInterval[0];
                interval[1] = lcg::largeInterval[1];
        }
        return new lcg::OU(mean, stddev, tau, ic, units, seed, interval, id);
}

lcg::Entity* OUNonStationaryFactory(string_dict& args)
{
        uint id;
        ullong seed;
        double tau, ic, interval[2];
        std::string units, intervalStr;
        id = lcg::GetIdFromDictionary(args);
        seed = lcg::GetSeedFromDictionary(args);
        if ( ! lcg::CheckAndExtractDouble(args, "tau", &tau) ||
             ! lcg::CheckAndExtractValue(args, "units", units) ||
             ! lcg::CheckAndExtractDouble(args, "initialCondition", &ic)) {
                lcg::Logger(lcg::Critical, "Unable to build a OU object.\n");
                return NULL;
        }
        if ( ! lcg::CheckAndExtractUnsignedLongLong(args, "seed", &seed))
                seed = 0;
        if (lcg::CheckAndExtractValue(args, "interval", intervalStr)) {
                size_t stop = intervalStr.find(",",0);
                if (stop == intervalStr.npos) {
                        lcg::Logger(lcg::Critical, "Error in the definition of the interval "
                                        "(which should be composed of two comma-separated values).\n");
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
                lcg::Logger(lcg::Debug, "Interval = [%g,%g].\n", interval[0], interval[1]);
        }
        else {
                interval[0] = lcg::largeInterval[0];
                interval[1] = lcg::largeInterval[1];
        }
        return new lcg::OUNonStationary(tau, ic, units, seed, interval, id);
}

namespace lcg {

OU::OU(double mean, double stddev, double tau, double initialCondition, std::string units, ullong seed, double interval[2], uint id)
        : DynamicalEntity(id), m_randn(NULL)
{
        // See the paper [Gillespie, 1994, PRE] for explanation of the meaning of parameters
        // and of the method of solution.
        OU_MEAN   = mean;
        OU_STDDEV = stddev;
        OU_TAU    = tau;
        OU_IC     = initialCondition;
        OU_CONST  = 2*OU_STDDEV*OU_STDDEV/OU_TAU;
        OU_MU     = exp(-GetGlobalDt()/OU_TAU);
        OU_COEFF  = sqrt(OU_CONST*OU_TAU/2 * (1-OU_MU*OU_MU));
        OU_START  = interval[0];
        OU_STOP   = interval[1];
        if (seed) {
                m_fixSeed = true;
                OU_SEED  = seed;
        }
        else {
                m_fixSeed = false;
        }

        m_state.push_back(0.0);         // m_state[0] -> eta
        m_state.push_back(0.0);         // m_state[1] -> auxiliary variable
        setName("OU");
        setUnits(units);
} 

OU::~OU() {
        if (m_randn)
                delete m_randn;
}

bool OU::initialise()
{
        OU_ETA = 0.0;
        OU_ETA_AUX = OU_IC;
        if (m_randn)
                delete m_randn;
        if (!m_fixSeed)
                OU_SEED = time(NULL);
        m_randn = new NormalRandom(0,1,OU_SEED); 
        return true;
}

void OU::evolve()
{
        double now = GetGlobalTime();
        if (now >= OU_START-GetGlobalDt()/2 && now <= OU_START) {
                OU_ETA = OU_IC;
        }
        else if (now > OU_START && now <= OU_STOP) {
                //OU_ETA_AUX = OU_IC*(1-OU_MU) + OU_MU*OU_ETA_AUX + OU_COEFF*m_randn->random();
                OU_ETA_AUX = OU_MEAN + OU_MU*(OU_ETA_AUX-OU_MEAN) + OU_COEFF*m_randn->random();
                OU_ETA = OU_ETA_AUX;
        }
        else {
                OU_ETA = 0.0;
        }
}

double OU::output()
{
        return OU_ETA;
}

OUNonStationary::OUNonStationary(double tau, double initialCondition, std::string units, ullong seed, double interval[2], uint id)
        : DynamicalEntity(id), m_randn(NULL)
{
        // See the paper [Gillespie, 1994, PRE] for explanation of the meaning of parameters
        // and of the method of solution.
        OU_TAU    = tau;
        OU_IC     = initialCondition;
        OU_MU     = exp(-GetGlobalDt()/OU_TAU);
        OU_START  = interval[0];
        OU_STOP   = interval[1];
        if (seed) {
                m_fixSeed = true;
                OU_SEED  = seed;
        }
        else {
                m_fixSeed = false;
        }

        m_state.push_back(0.0);         // m_state[0] -> eta
        m_state.push_back(0.0);         // m_state[1] -> auxiliary variable
        setName("OUNonStationary");
        setUnits(units);
} 

OUNonStationary::~OUNonStationary() {
        if (m_randn)
                delete m_randn;
}

bool OUNonStationary::initialise()
{
        if (m_inputs.size() != 2) {
                Logger(Critical, "A OUNonStationaty object requires exactly two inputs, "
                                "one for the mean and one for the standard deviation.\n");
                return false;
        }

        OU_ETA = 0.0;
        OU_ETA_AUX = OU_IC;
        if (m_randn)
                delete m_randn;
        if (!m_fixSeed)
                OU_SEED = time(NULL);
        m_randn = new NormalRandom(0,1,OU_SEED); 
        return true;
}

void OUNonStationary::evolve()
{
        double now = GetGlobalTime();
        if (now >= OU_START-GetGlobalDt()/2 && now <= OU_START) {
                OU_ETA = OU_IC;
        }
        else if (now > OU_START && now <= OU_STOP) {
                double c = 2*OUNS_STDDEV*OUNS_STDDEV/OU_TAU;
                OU_ETA_AUX = OUNS_MEAN + OU_MU*(OU_ETA_AUX-OUNS_MEAN) + sqrt(0.5*c*OU_TAU*(1-OU_MU*OU_MU))*m_randn->random();
                OU_ETA = OU_ETA_AUX;
        }
        else {
                OU_ETA = 0.0;
        }
}

double OUNonStationary::output()
{
        return OU_ETA;
}

} // namespace lcg

