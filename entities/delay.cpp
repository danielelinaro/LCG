#include <math.h>
#include "delay.h"

lcg::Entity* DelayFactory(string_dict& args)
{
        uint id, nSamples;
        double delay;
        id = lcg::GetIdFromDictionary(args);
        if ( ! lcg::CheckAndExtractUnsignedInteger(args, "nSamples", &nSamples)) {
                if (lcg::CheckAndExtractDouble(args, "delay", &delay)) {
                        return new lcg::Delay(delay, id);
                }
                else {
                        lcg::Logger(lcg::Critical, "Unable to build a delay.\n");
                        return NULL;
                }
        }
        return new lcg::Delay(nSamples, id);
}

namespace lcg {

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

} // namespace lcg

