#include "analog_io.h"
#include "utils.h"

#ifdef HAVE_LIBCOMEDI
#include <string.h>

dynclamp::Entity* AnalogInputFactory(dictionary& args)
{
        uint inputSubdevice, readChannel, id;
        std::string deviceFile;
        double inputConversionFactor, dt;

        dynclamp::GetIdAndDtFromDictionary(args, &id, &dt);

        if ( ! dynclamp::CheckAndExtractValue(args, "deviceFile", deviceFile) ||
             ! dynclamp::CheckAndExtractUnsignedInteger(args, "inputSubdevice", &inputSubdevice) ||
             ! dynclamp::CheckAndExtractUnsignedInteger(args, "readChannel", &readChannel) ||
             ! dynclamp::CheckAndExtractDouble(args, "inputConversionFactor", &inputConversionFactor)) {
                dynclamp::Logger(dynclamp::Debug, "AnalogInputFactory: missing parameter.\n");
                dictionary::iterator it;
                for (it = args.begin(); it != args.end(); it++)
                        dynclamp::Logger(dynclamp::Debug, "%s -> %s\n", (*it).first.c_str(), (*it).second.c_str());
                return NULL;
        }

        return new dynclamp::AnalogInput(deviceFile.c_str(), inputSubdevice, readChannel, inputConversionFactor, id, dt);
}

dynclamp::Entity* AnalogOutputFactory(dictionary& args)
{
        uint outputSubdevice, writeChannel, id;
        std::string deviceFile;
        double outputConversionFactor, dt;

        dynclamp::GetIdAndDtFromDictionary(args, &id, &dt);

        if ( ! dynclamp::CheckAndExtractValue(args, "deviceFile", deviceFile) ||
             ! dynclamp::CheckAndExtractUnsignedInteger(args, "outputSubdevice", &outputSubdevice) ||
             ! dynclamp::CheckAndExtractUnsignedInteger(args, "writeChannel", &writeChannel) ||
             ! dynclamp::CheckAndExtractDouble(args, "outputConversionFactor", &outputConversionFactor)) {
                dynclamp::Logger(dynclamp::Debug, "AnalogOutputFactory: missing parameter.\n");
                return NULL;
        }

        return new dynclamp::AnalogOutput(deviceFile.c_str(), outputSubdevice, writeChannel, outputConversionFactor, id, dt);
}

namespace dynclamp {

ComediAnalogIO::ComediAnalogIO(const char *deviceFile, uint subdevice, uint channel)
        : m_subdevice(subdevice), m_channel(channel)
{
        strncpy(m_deviceFile, deviceFile, 30);
        if (!openDevice())
                throw "Unable to open communication with the DAQ board.";
}

ComediAnalogIO::~ComediAnalogIO()
{
        closeDevice();
}

bool ComediAnalogIO::openDevice()
{
        m_device = comedi_open(m_deviceFile);
        if(m_device == NULL) {
                comedi_perror(m_deviceFile);
                return false;
        }

        int nranges = comedi_get_n_ranges(m_device, m_subdevice, m_channel);
        Logger(Debug, "%s, subdevice #%d, channel #%d has %d range%s.\n", m_deviceFile,
                        m_subdevice, m_channel, nranges, (nranges > 1 ? "s" : ""));

        // Here, we are using the first (and widest) range, which corresponds (at least on NI boards)
        // to [-10,+10] volts. One can use the function comedi_find_range, to find the index of
        // the range that will be enough.
        /* get physical data range for subdevice (min, max, phys. units) */
        m_dataRange = comedi_get_range(m_device, m_subdevice, m_channel, wideRange);
        Logger(Debug, "Range for 0x%x (%s):\n\tmin = %g\n\tmax = %g\n", m_dataRange,
                        (m_dataRange->unit == UNIT_volt ? "volts" : "milliamps"),
                        m_dataRange->min, m_dataRange->max);
        if(m_dataRange == NULL) {
                comedi_perror(m_deviceFile);
                comedi_close(m_device);
                return false;
        }
        
        /* read max data values */
        m_maxData = comedi_get_maxdata(m_device, m_subdevice, m_channel);
        Logger(Debug, "Max data = %ld\n", m_maxData);

        return true;
}

void ComediAnalogIO::closeDevice()
{
        comedi_close(m_device);
}

const char* ComediAnalogIO::deviceFile() const
{
        return m_deviceFile;
}

const uint ComediAnalogIO::subdevice() const
{
        return m_subdevice;
}

const uint ComediAnalogIO::channel() const
{
        return m_channel;
}

//~~~

ComediAnalogInput::ComediAnalogInput(const char *deviceFile, uint inputSubdevice,
                                     uint readChannel, double inputConversionFactor)
        : ComediAnalogIO(deviceFile, inputSubdevice, readChannel),
          m_inputConversionFactor(inputConversionFactor)
{}

double ComediAnalogInput::inputConversionFactor() const
{
        return m_inputConversionFactor;
}

double ComediAnalogInput::read()
{
        lsampl_t sample;
#ifdef HEKA
        comedi_data_read(m_device, m_subdevice, m_channel, wideRange, groundReferencedSingleEnded, &sample);
#else
        comedi_data_read(m_device, m_subdevice, m_channel, wideRange, nonReferencedSingleEnded, &sample);
#endif
        double dt = GetGlobalDt(), now = GetGlobalTime();
        if (now > 1-dt/2 && now < 1+dt/2)
                Logger(Debug, "read: 0x%x\n", sample);
        return comedi_to_phys(sample, m_dataRange, m_maxData) * m_inputConversionFactor;
}

//~~~

ComediAnalogOutput::ComediAnalogOutput(const char *deviceFile, uint outputSubdevice,
                                       uint writeChannel, double outputConversionFactor)
        : ComediAnalogIO(deviceFile, outputSubdevice, writeChannel),
          m_outputConversionFactor(outputConversionFactor)
{}

ComediAnalogOutput::~ComediAnalogOutput()
{
        write(0.0);
}

double ComediAnalogOutput::outputConversionFactor() const
{
        return m_outputConversionFactor;
}

/**
 * Write the argument to the output of the DAQ board.
 */
void ComediAnalogOutput::write(double data)
{
        lsampl_t sample; 
        sample = comedi_from_phys(data*m_outputConversionFactor, m_dataRange, m_maxData);
        double dt = GetGlobalDt(), now = GetGlobalTime();
        if (now > 1-dt/2 && now < 1+dt/2)
                Logger(Debug, "written: 0x%x\n", sample);
#ifdef HEKA
        comedi_data_write(m_device, m_subdevice, m_channel, wideRange, groundReferencedSingleEnded, sample);
#else
        comedi_data_write(m_device, m_subdevice, m_channel, wideRange, nonReferencedSingleEnded, sample);
#endif
}

//~~~

AnalogInput::AnalogInput(const char *deviceFile, uint inputSubdevice,
                         uint readChannel, double inputConversionFactor,
                         uint id, double dt)
        : Entity(id, dt), m_data(0.0),
          m_input(deviceFile, inputSubdevice, readChannel, inputConversionFactor)
{}

void AnalogInput::step()
{
        m_data = m_input.read();
}

double AnalogInput::output() const
{
        return m_data;
}

//~~~

AnalogOutput::AnalogOutput(const char *deviceFile, uint outputSubdevice,
                           uint writeChannel, double outputConversionFactor,
                           uint id, double dt)
        : Entity(id, dt),
          m_output(deviceFile, outputSubdevice, writeChannel, outputConversionFactor)
{}

AnalogOutput::~AnalogOutput()
{
        m_output.write(0.0);
}

void AnalogOutput::step()
{
        uint i, n = m_inputs.size();
        m_data = 0.0;
        for (i=0; i<n; i++)
                m_data += m_inputs[i];
        m_output.write(m_data);
}

double AnalogOutput::output() const
{
        return m_data;
}

} // namespace dynclamp

#endif // HAVE_LIBCOMEDI

