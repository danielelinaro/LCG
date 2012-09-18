#include <math.h>
#include "delay.h"
#include "engine.h"

dynclamp::Entity* DelayFactory(string_dict& args)
{
        uint id, nSamples;
        double delay;
        id = dynclamp::GetIdFromDictionary(args);
        if ( ! dynclamp::CheckAndExtractUnsignedInteger(args, "nSamples", &nSamples)) {
                if (dynclamp::CheckAndExtractDouble(args, "delay", &delay)) {
                        return new dynclamp::Delay(delay, id);
                }
                else {
                        dynclamp::Logger(dynclamp::Critical, "Unable to build a delay.\n");
                        return NULL;
                }
        }
        return new dynclamp::Delay(nSamples, id);
}

namespace dynclamp {

Delay::Delay(uint nSamples, uint id)
        : Entity(id), m_bufferPosition(0)
{
        if (nSamples == 0)
                throw "The number of delay samples must be greater than 0.";

        m_bufferLength = nSamples + 1;
        allocateBuffer();
        setName("Delay");
}

Delay::Delay(double delay, uint id)
        : Entity(id)
{
        if (delay <= 0)
                throw "The delay must be greater than 0.";

        m_bufferLength = ceil(delay / GetGlobalDt()) + 1;
        allocateBuffer();
}

Delay::~Delay()
{
        delete m_buffer;
}

void Delay::allocateBuffer()
{
        m_buffer = new double[m_bufferLength];
}

bool Delay::initialise()
{
        m_bufferPosition = 0;
        for (uint i=0; i<m_bufferLength; i++)
                m_buffer[i] = 0.0;
        return true;
}

void Delay::step()
{
        m_buffer[m_bufferPosition] = m_inputs[0];
        m_bufferPosition = (m_bufferPosition+1) % m_bufferLength;
}

double Delay::output()
{
        return m_buffer[(m_bufferPosition+1) % m_bufferLength];
}

} // namespace dynclamp

