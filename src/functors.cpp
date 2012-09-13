#include "functors.h"
#include "randlib.h"
#include "engine.h"

dynclamp::Entity* SobolDelayFactory(dictionary& args)
{
        return new dynclamp::SobolDelay(dynclamp::GetIdFromDictionary(args));
}

dynclamp::Entity* PhasicDelayFactory(dictionary& args)
{

        uint id;
        double delay = 0.0;

        id = dynclamp::GetIdFromDictionary(args);
        if (! dynclamp::CheckAndExtractDouble(args, "delay", &delay)) 
                dynclamp::Logger(dynclamp::Important, "PhasicDelay(%d): Using a phasic delay of zero!\n",id);
        return new dynclamp::PhasicDelay(id,delay);
}

namespace dynclamp {

Functor::Functor(uint id)
        : Entity(id)
{
        setName("Functor");
}

void Functor::step()
{}

double Functor::output() const
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

PhasicDelay::PhasicDelay(uint id, double phase)
        : Functor(id), m_phase(phase)
{
        setName("PhasicDelay");
		Logger(Important,"PhasicDelay(%d): Using a delay of %3.3f.\n", id, phase);
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
        return m_phase*y;
}

}//namespace dynclamp

