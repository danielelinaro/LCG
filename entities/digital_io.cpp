#include <string.h>
#include "digital_io.h"
#include "utils.h"

lcg::Entity* DigitalInputFactory(string_dict& args)
{
        uint inputSubdevice, readChannel, id;
        std::string deviceFile,units;
        
	id = lcg::GetIdFromDictionary(args);

        if ( ! lcg::CheckAndExtractValue(args, "deviceFile", deviceFile) ||
             ! lcg::CheckAndExtractUnsignedInteger(args, "inputSubdevice", &inputSubdevice) ||
             ! lcg::CheckAndExtractUnsignedInteger(args, "readChannel", &readChannel)) {
                lcg::Logger(lcg::Debug, "AnalogInputFactory: missing parameter.\n");
                string_dict::iterator it;
                lcg::Logger(lcg::Critical, "Unable to build an digital input.\n");
                return NULL;
        }

        if (! lcg::CheckAndExtractValue(args, "units", units)) {
                units = "Boolean";
        }
        return new lcg::DigitalInput(deviceFile.c_str(), inputSubdevice, readChannel,
					units, id);
}

lcg::Entity* DigitalOutputFactory(string_dict& args)
{
        uint outputSubdevice, writeChannel, reference, id;
        std::string deviceFile,units;

        id = lcg::GetIdFromDictionary(args);

        if ( ! lcg::CheckAndExtractValue(args, "deviceFile", deviceFile) ||
             ! lcg::CheckAndExtractUnsignedInteger(args, "outputSubdevice", &outputSubdevice) ||
             ! lcg::CheckAndExtractUnsignedInteger(args, "writeChannel", &writeChannel)) {
                lcg::Logger(lcg::Critical, "Unable to build an digital output.\n");
                return NULL;
        }

        if (! lcg::CheckAndExtractValue(args, "units", units)) {
                units = "Boolean";
        }
        return new lcg::DigitalOutput(deviceFile.c_str(), outputSubdevice, writeChannel,
                                         units, id);
}

namespace lcg {

DigitalInput::DigitalInput(const char *deviceFile, uint inputSubdevice,
                         uint readChannel,
			 const std::string& units,
                         uint id)
        : Entity(id),
#if defined(HAVE_LIBCOMEDI)
          m_input(deviceFile, inputSubdevice, readChannel)
#endif
{
        setName("DigitalInput");
        setUnits(units);
}

bool DigitalInput::initialise()
{
        if (! m_input.initialise())
                return false;
        m_data = m_input.read();
        return true;
}

void DigitalInput::step()
{
        m_data = m_input.read();
}

void DigitalInput::firstStep()
{
	m_input.initialise(); // Make sure it is configured
        step();
}

double DigitalInput::output()
{
        return m_data;
}

//~~~

DigitalOutput::DigitalOutput(const char *deviceFile, uint outputSubdevice,
                           uint writeChannel,
			   const std::string& units, uint id)
        : Entity(id),
#if defined(HAVE_LIBCOMEDI)
          m_output(deviceFile, outputSubdevice, writeChannel)
#endif
{
        setName("DigitalOutput");
        setUnits(units);
}

DigitalOutput::~DigitalOutput()
{
        terminate();
}

void DigitalOutput::terminate()
{
}

bool DigitalOutput::initialise()
{
        if (! m_output.initialise())
                return false;
        return true;
}

void DigitalOutput::firstStep()
{
	m_output.initialise(); // make sure it is configured.
	step();
}
void DigitalOutput::step()
{
        uint i, n = m_inputs.size();
        m_data = 0.0;
        for (i=0; i<n; i++)
                m_data += m_inputs[i];
        m_output.write(m_data);
}

double DigitalOutput::output()
{
        return m_data;
}

} // namespace lcg

