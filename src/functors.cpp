#include "functors.h"
#include "engine.h"
#include <sstream>

dynclamp::Entity* SobolDelayFactory(string_dict& args)
{
        return new dynclamp::SobolDelay(dynclamp::GetIdFromDictionary(args));
}

dynclamp::Entity* PhasicDelayFactory(string_dict& args)
{
        uint id;
        double delay;
        id = dynclamp::GetIdFromDictionary(args);
        if (! dynclamp::CheckAndExtractDouble(args, "delay", &delay)) {
                dynclamp::Logger(dynclamp::Important, "PhasicDelay(%d): Using a phasic delay of zero!\n", id);
                delay = 0.0;
        }
        return new dynclamp::PhasicDelay(delay, id);
}

dynclamp::Entity* RandomDelayFactory(string_dict& args)
{
        uint id;
        ullong seed;
        std::string intervalStr;
        double interval[2], mean, stddev;
        id = dynclamp::GetIdFromDictionary(args);

        if (! dynclamp::CheckAndExtractUnsignedLongLong(args, "seed", &seed))
                seed = time(NULL);

        if (dynclamp::CheckAndExtractValue(args, "interval", intervalStr)) {
                size_t stop = intervalStr.find(",",0);
                if (stop == intervalStr.npos) {
                        dynclamp::Logger(dynclamp::Critical, "Error in the definition of the interval "
                                        "(which should be composed of two comma-separated values).\n");
                        return NULL;
                }
                std::stringstream ss0(intervalStr.substr(0,stop)), ss1(intervalStr.substr(stop+1));
                ss0 >> interval[0];
                ss1 >> interval[1];
                dynclamp::Logger(dynclamp::Debug, "Interval = [%g,%g].\n", interval[0], interval[1]);
                return new dynclamp::RandomDelay(interval, seed, id);
        }

        if (dynclamp::CheckAndExtractDouble(args, "mean", &mean) &&
                 dynclamp::CheckAndExtractDouble(args, "stddev", &stddev)) {
                return new dynclamp::RandomDelay(mean, stddev, seed, id);
        }
        
        dynclamp::Logger(dynclamp::Important, "Unable to build a RandomDelay.\n");
        return NULL;
}

namespace dynclamp {

Functor::Functor(uint id) : Entity(id)
{
        setName("Functor");
}

void Functor::step()
{}

double Functor::output()
{
        return 0.0;
}

bool Functor::initialise()
{
        return true;
}

SobolDelay::SobolDelay(uint id)
        : Functor(id), m_numberOfSobolSequences(-1)
{
        setName("SobolDelay");
        setUnits("s");
}

bool SobolDelay::initialise()
{
        m_numberOfSobolSequences = -1;
        sobseq(&m_numberOfSobolSequences, NULL);
        m_numberOfSobolSequences = 1;
        return true;
}

double SobolDelay::operator()()
{
        float x;
        double y;
        sobseq(&m_numberOfSobolSequences, &x);
        if (!ConvertUnits(m_inputs[0], &y, pre()[0]->units(), "s"))
                throw "Unable to convert.";
        return x*y;
}

PhasicDelay::PhasicDelay(double phase, uint id)
        : Functor(id)
{
        m_parameters["phase"] = phase;
        setName("PhasicDelay");
        setUnits("s");
	Logger(Debug, "PhasicDelay(%d): Using a delay of %3.3f.\n", id, m_parameters["phase"]);
}

bool PhasicDelay::initialise()
{
        return true;
}

double PhasicDelay::operator()()
{
        double y;
        if (!ConvertUnits(m_inputs[0], &y, pre()[0]->units(), "s"))
                throw "Unable to convert.";
        return m_parameters["phase"] * y;
}

//~~~

RandomDelay::RandomDelay(double interval[2], ullong seed, uint id)
        : Functor(id), rand(new UniformRandom(seed))
{
        m_parameters["min"] = interval[0];
        m_parameters["max"] = interval[1];
        m_parameters["seed"] = (double) seed;
        setName("RandomDelay");
        setUnits("s");
        m_coeffs[0] = interval[0];
        m_coeffs[1] = interval[1] - interval[0];
}

RandomDelay::RandomDelay(double mean, double stddev, ullong seed, uint id) 
        : Functor(id), rand(new NormalRandom(0.,1.,seed))
{
        m_parameters["mean"] = mean;
        m_parameters["stddev"] = stddev;
        m_parameters["seed"] = (double) seed;
        setName("RandomDelay");
        setUnits("s");
        m_coeffs[0] = mean;
        m_coeffs[1] = stddev;
}

RandomDelay::~RandomDelay()
{
        delete rand;
}

bool RandomDelay::initialise()
{
        return true;
}

double RandomDelay::operator()()
{
        double delay;
        do {
                delay = m_coeffs[0] + m_coeffs[1]*rand->random();
        } while (delay <= 0);
        return delay;
}

}//namespace dynclamp

