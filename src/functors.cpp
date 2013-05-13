#include "functors.h"
#include "engine.h"
#include <sstream>

lcg::Entity* SobolDelayFactory(string_dict& args)
{
        uint id, startSample;
        id = lcg::GetIdFromDictionary(args);
        if (! lcg::CheckAndExtractUnsignedInteger(args, "startSample", &startSample))
                startSample = 0;
		lcg::Logger(lcg::Important, "Using start sample [%d].\n", startSample);
        return new lcg::SobolDelay(startSample, id);
}

lcg::Entity* PhasicDelayFactory(string_dict& args)
{
        uint id;
        double delay;
        id = lcg::GetIdFromDictionary(args);
        if (! lcg::CheckAndExtractDouble(args, "delay", &delay)) {
                lcg::Logger(lcg::Important, "PhasicDelay(%d): Using a phasic delay of zero!\n", id);
                delay = 0.0;
        }
        return new lcg::PhasicDelay(delay, id);
}

lcg::Entity* RandomDelayFactory(string_dict& args)
{
        uint id;
        ullong seed;
        std::string intervalStr;
        double interval[2], mean, stddev;
        id = lcg::GetIdFromDictionary(args);

        if (! lcg::CheckAndExtractUnsignedLongLong(args, "seed", &seed))
                seed = time(NULL);

        if (lcg::CheckAndExtractValue(args, "interval", intervalStr)) {
                size_t stop = intervalStr.find(",",0);
                if (stop == intervalStr.npos) {
                        lcg::Logger(lcg::Critical, "Error in the definition of the interval "
                                        "(which should be composed of two comma-separated values).\n");
                        return NULL;
                }
                std::stringstream ss0(intervalStr.substr(0,stop)), ss1(intervalStr.substr(stop+1));
                ss0 >> interval[0];
                ss1 >> interval[1];
                lcg::Logger(lcg::Debug, "Interval = [%g,%g].\n", interval[0], interval[1]);
                return new lcg::RandomDelay(interval, seed, id);
        }

        if (lcg::CheckAndExtractDouble(args, "mean", &mean) &&
                 lcg::CheckAndExtractDouble(args, "stddev", &stddev)) {
                return new lcg::RandomDelay(mean, stddev, seed, id);
        }
        
        lcg::Logger(lcg::Important, "Unable to build a RandomDelay.\n");
        return NULL;
}

namespace lcg {

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

SobolDelay::SobolDelay(uint startSample, uint id)
        : Functor(id), m_numberOfSobolSequences(-1), m_startSample(startSample)
{
        setName("SobolDelay");
        setUnits("s");
}

bool SobolDelay::initialise()
{
        m_numberOfSobolSequences = -1;
        sobseq(&m_numberOfSobolSequences, NULL);
        m_numberOfSobolSequences = 1;
		for (uint i=0; i<m_startSample; i++)
			this->operator()();
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

}//namespace lcg

