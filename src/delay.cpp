#include "delay.h"
#include <math.h>

dynclamp::Entity* DelayFactory(dictionary& args)
{
        uint id, nSamples;
        double dt, delay;
        dynclamp::GetIdAndDtFromDictionary(args, &id, &dt);
        if ( ! dynclamp::CheckAndExtractUnsignedInteger(args, "nSamples", &nSamples)) {
                if (dynclamp::CheckAndExtractDouble(args, "delay", &delay))
                        return new dynclamp::Delay(delay, id, dt);
                else
                        return NULL;
        }
        return new dynclamp::Delay(nSamples, id, dt);
}

namespace dynclamp {

Delay::Delay(uint nSamples, uint id, double dt)
        : Entity(id, dt), m_bufferPosition(0)
{
        if (nSamples == 0)
                throw "The number of delay samples must be greater than 0.";

        m_bufferLength = nSamples + 1;
        allocateAndInitialiseBuffer();
}

Delay::Delay(double delay, uint id, double dt)
        : Entity(id, dt), m_bufferPosition(0)
{
        if (delay <= 0)
                throw "The delay must be greater than 0.";

        m_bufferLength = ceil(delay / GetGlobalDt()) + 1;
        allocateAndInitialiseBuffer();
}

void Delay::allocateAndInitialiseBuffer()
{
        m_buffer = new double[m_bufferLength];
        for (uint i=0; i<m_bufferLength; i++)
                m_buffer[i] = 0.0;
}

Delay::~Delay()
{
        delete m_buffer;
}

void Delay::step()
{
        m_buffer[m_bufferPosition] = m_inputs[0];
        m_bufferPosition = (m_bufferPosition+1) % m_bufferLength;
}

double Delay::output() const
{
        return m_buffer[(m_bufferPosition+1) % m_bufferLength];
}

} // namespace dynclamp

