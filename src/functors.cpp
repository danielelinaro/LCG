#include "functors.h"
#include "randlib.h"
#include "engine.h"

dynclamp::Entity* SobolDelayFactory(dictionary& args)
{
        return new dynclamp::SobolDelay(dynclamp::GetIdFromDictionary(args));
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

}

