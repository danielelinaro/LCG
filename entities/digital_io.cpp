#include <string.h>
#include "digital_io.h"
#include "utils.h"

lcg::Entity* DigitalInputFactory(string_dict& args)
{
        uint inputSubdevice, readChannel, id;
        std::string deviceFile, units, eventStr;
        lcg::EventType eventToSend;
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
        if (lcg::CheckAndExtractValue(args, "eventToSend", eventStr)) {
                int i=0;
                while(i < NUMBER_OF_EVENT_TYPES && strcasecmp(eventStr.c_str(), lcg::eventTypeNames[i].c_str()))
                    i++;
                if (i == NUMBER_OF_EVENT_TYPES) {
                    lcg::Logger(lcg::Critical, "DigitalInput(%d): Unknown event type [%s].\n", id, eventStr.c_str());
                    return NULL;
                }
                eventToSend = static_cast<lcg::EventType>(i);
		}
		else {
			eventToSend = lcg::DIGITAL_RISE;
        }
        return new lcg::DigitalInput(deviceFile.c_str(), inputSubdevice, readChannel,
					units, eventToSend, id);
}

lcg::Entity* DigitalOutputFactory(string_dict& args)
{
        uint outputSubdevice, writeChannel, reference, id;
        std::string deviceFile,units, eventStr;
        lcg::EventType eventToSend;
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
        if (lcg::CheckAndExtractValue(args, "eventToSend", eventStr)) {
                int i=0;
                while(i < NUMBER_OF_EVENT_TYPES && strcasecmp(eventStr.c_str(), lcg::eventTypeNames[i].c_str()))
                    i++;
                if (i == NUMBER_OF_EVENT_TYPES) {
                    lcg::Logger(lcg::Critical, "DigitalOutput(%d): Unknown event type [%s].\n", id, eventStr.c_str());
                    return NULL;
                }
                eventToSend = static_cast<lcg::EventType>(i);
		}
		else {
			eventToSend = lcg::DIGITAL_RISE;
        }
        return new lcg::DigitalOutput(deviceFile.c_str(), outputSubdevice, writeChannel,
                                         units, eventToSend, id);
}

namespace lcg {

DigitalInput::DigitalInput(const char *deviceFile, uint inputSubdevice,
                         uint readChannel, const std::string& units,
                         EventType eventToSend, uint id)
        : Entity(id),
#if defined(HAVE_LIBCOMEDI)
	m_input(deviceFile, inputSubdevice, readChannel),
#endif
	m_eventToSend(eventToSend)
{
        setName("DigitalInput");
        setUnits(units);
		m_previous = 0;
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
	//Rising crossing
	if (m_data - m_previous > 0) {
		switch (m_eventToSend) {
			case DIGITAL_RISE:
				emitEvent(new DigitalRiseEvent(this));
				break;
			case STOPRUN:
				emitEvent(new StopRunEvent(this));
				Logger(Important, "Simulation terminated by DigitalInput(%d).\n", id());
				break;
			case RESET:
				emitEvent(new ResetEvent(this));
				break;
			case TOGGLE:
				emitEvent(new ToggleEvent(this));
				break;
			case TRIGGER: 
				emitEvent(new TriggerEvent(this));
				break;
			case SPIKE:
				emitEvent(new SpikeEvent(this));
				break;
			default:
				Logger(Important, "DigitalInput(%d): Can't send event.\n", id());
				break;
		}
	}
	m_previous = m_data;
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
			   const std::string& units, EventType eventToSend, uint id)
        : Entity(id),
#if defined(HAVE_LIBCOMEDI)
          m_output(deviceFile, outputSubdevice, writeChannel),
#endif
	m_eventToSend(eventToSend)
{
        setName("DigitalOutput");
        setUnits(units);
	m_previous = 0;
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
	if (m_data - m_previous > 0) {
		switch (m_eventToSend) {
			case DIGITAL_RISE:
				emitEvent(new DigitalRiseEvent(this));
				break;
			case STOPRUN:
				emitEvent(new StopRunEvent(this));
				Logger(Important, "Simulation terminated by DigitalOutput(%d).\n", id());
				break;
			case RESET:
				emitEvent(new ResetEvent(this));
				break;
			case TOGGLE:
				emitEvent(new ToggleEvent(this));
				break;
			case TRIGGER: 
				emitEvent(new TriggerEvent(this));
				break;
			case SPIKE:
				emitEvent(new SpikeEvent(this));
				break;
			default:
				Logger(Important, "DigitalOutput(%d): Can't send event.\n", id());
				break;
		}
	}
	m_previous = m_data;
}

double DigitalOutput::output()
{
        return m_data;
}

} // namespace lcg

