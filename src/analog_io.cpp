#include "analog_io.h"
#ifdef HAVE_LIBCOMEDI
#include <string.h>

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

        /* get physical data range for subdevice (min, max, phys. units) */
        m_dataRange = comedi_get_range(m_device, m_subdevice, m_channel, AREF_GROUND);
        if(m_dataRange == NULL) {
                comedi_perror(m_deviceFile);
                comedi_close(m_device);
                return false;
        }
        
        /* read max data values */
        m_maxData = comedi_get_maxdata(m_device, m_subdevice, m_channel);

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
        comedi_data_read(m_device, m_subdevice, m_channel, RANGE, AREF_GROUND, &sample);
        return comedi_to_phys(sample, m_dataRange, m_maxData) * m_inputConversionFactor;
}

//~~~

ComediAnalogOutput::ComediAnalogOutput(const char *deviceFile, uint outputSubdevice,
                                       uint writeChannel, double outputConversionFactor)
        : ComediAnalogIO(deviceFile, outputSubdevice, writeChannel),
          m_outputConversionFactor(outputConversionFactor)
{}

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
        comedi_data_write(m_device, m_subdevice, m_channel, RANGE, AREF_GROUND, sample);
}

} // namespace dynclamp

#endif // HAVE_LIBCOMEDI

