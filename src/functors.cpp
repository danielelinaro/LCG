#include "functors.h"
#include "randlib.h"
#include "engine.h"

dynclamp::Entity* SobolDelayFactory(string_dict& args)
{
        return new dynclamp::SobolDelay(dynclamp::GetIdFromDictionary(args));
}

dynclamp::Entity* PhasicDelayFactory(string_dict& args)
{

        uint id;
        double delay = 0.0;

        id = dynclamp::GetIdFromDictionary(args);
        if (! dynclamp::CheckAndExtractDouble(args, "delay", &delay)) 
                dynclamp::Logger(dynclamp::Important, "PhasicDelay(%d): Using a phasic delay of zero!\n", id);
        return new dynclamp::PhasicDelay(delay, id);
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

}//namespace dynclamp

