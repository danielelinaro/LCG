#include "analog_io.h"
#include "utils.h"

#ifdef HAVE_LIBCOMEDI
#include <string.h>

dynclamp::Entity* AnalogInputFactory(dictionary& args)
{
        uint inputSubdevice, readChannel, range, reference, id;
        std::string deviceFile, rangeStr, referenceStr;
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

        if (! dynclamp::CheckAndExtractValue(args, "range", rangeStr)) {
                range = PLUS_MINUS_TEN;
        }
        else {
                if (rangeStr.compare("PlusMinusTen") == 0 ||
                    rangeStr.compare("[-10,+10]") == 0 ||
                    rangeStr.compare("+-10") == 0) {
                        range = PLUS_MINUS_TEN;
                }
                else if (rangeStr.compare("PlusMinusFive") == 0 ||
                         rangeStr.compare("[-5,+5]") == 0 ||
                         rangeStr.compare("+-5") == 0) {
                        range = PLUS_MINUS_FIVE;
                }
                else if (rangeStr.compare("PlusMinusOne") == 0 ||
                         rangeStr.compare("[-1,+1]") == 0 ||
                         rangeStr.compare("+-1") == 0) {
                        range = PLUS_MINUS_ONE;
                }
                else if (rangeStr.compare("PlusMinusZeroPointTwo") == 0 ||
                         rangeStr.compare("[-0.2,+0.2]") == 0 ||
                         rangeStr.compare("+-0.2") == 0) {
                        range = PLUS_MINUS_ZERO_POINT_TWO;
                }
                else {
                        dynclamp::Logger(dynclamp::Critical, "Unknown input range: [%s].\n", rangeStr.c_str());
                        return NULL;
                }
        }

        if (! dynclamp::CheckAndExtractValue(args, "reference", referenceStr)) {
                reference = GRSE;
        }
        else {
                if (referenceStr.compare("GRSE") == 0) {
                        reference = GRSE;
                }
                else if (referenceStr.compare("NRSE") == 0) {
                        reference = NRSE;
                }
                else {
                        dynclamp::Logger(dynclamp::Critical, "Unknown reference mode: [%s].\n", referenceStr.c_str());
                        return NULL;
                }
        }

        return new dynclamp::AnalogInput(deviceFile.c_str(), inputSubdevice, readChannel,
                        inputConversionFactor, range, reference, id, dt);
}

dynclamp::Entity* AnalogOutputFactory(dictionary& args)
{
        uint outputSubdevice, writeChannel, reference, id;
        std::string deviceFile, referenceStr;
        double outputConversionFactor, dt;

        dynclamp::GetIdAndDtFromDictionary(args, &id, &dt);

        if ( ! dynclamp::CheckAndExtractValue(args, "deviceFile", deviceFile) ||
             ! dynclamp::CheckAndExtractUnsignedInteger(args, "outputSubdevice", &outputSubdevice) ||
             ! dynclamp::CheckAndExtractUnsignedInteger(args, "writeChannel", &writeChannel) ||
             ! dynclamp::CheckAndExtractDouble(args, "outputConversionFactor", &outputConversionFactor)) {
                dynclamp::Logger(dynclamp::Debug, "AnalogOutputFactory: missing parameter.\n");
                return NULL;
        }

        if (! dynclamp::CheckAndExtractValue(args, "reference", referenceStr)) {
                reference = GRSE;
        }
        else {
                if (referenceStr.compare("GRSE") == 0) {
                        reference = GRSE;
                }
                else if (referenceStr.compare("NRSE") == 0) {
                        reference = NRSE;
                }
                else {
                        dynclamp::Logger(dynclamp::Critical, "Unknown reference mode: [%s].\n", referenceStr.c_str());
                        return NULL;
                }
        }

        return new dynclamp::AnalogOutput(deviceFile.c_str(), outputSubdevice, writeChannel,
                        outputConversionFactor, reference, id, dt);
}

namespace dynclamp {

ComediAnalogIO::ComediAnalogIO(const char *deviceFile, uint subdevice, uint channel,
                               uint range, uint aref)
        : m_subdevice(subdevice), m_channel(channel), m_range(range), m_aref(aref)
{
        Logger(Debug, "ComediAnalogIO::ComediAnalogIO()\n");
        strncpy(m_deviceFile, deviceFile, 30);
        if (!openDevice())
                throw "Unable to open communication with the DAQ board.";
}

ComediAnalogIO::~ComediAnalogIO()
{
        Logger(Debug, "ComediAnalogIO::~ComediAnalogIO()\n");
        comedi_close(m_device);
}

bool ComediAnalogIO::openDevice()
{
        Logger(Debug, "ComediAnalogIO::openDevice()\n");
        m_device = comedi_open(m_deviceFile);
        if(m_device == NULL) {
                comedi_perror(m_deviceFile);
                return false;
        }

        /*
        int nranges = comedi_get_n_ranges(m_device, m_subdevice, m_channel);
        Logger(Debug, "%s, subdevice #%d, channel #%d has %d range%s.\n", m_deviceFile,
                        m_subdevice, m_channel, nranges, (nranges > 1 ? "s" : ""));
        */

        /* get physical data range for subdevice (min, max, phys. units) */
        m_dataRange = comedi_get_range(m_device, m_subdevice, m_channel, m_range);
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

const char* ComediAnalogIO::deviceFile() const
{
        return m_deviceFile;
}

uint ComediAnalogIO::subdevice() const
{
        return m_subdevice;
}

uint ComediAnalogIO::channel() const
{
        return m_channel;
}

uint ComediAnalogIO::range() const
{
        return m_range;
}

uint ComediAnalogIO::reference() const
{
        return m_aref;
}

//~~~

ComediAnalogIOSoftCal::ComediAnalogIOSoftCal(const char *deviceFile, uint subdevice, uint channel,
                                             uint range, uint aref)
        : ComediAnalogIO(deviceFile, subdevice, channel, range, aref)
{
        Logger(Debug, "ComediAnalogIOSoftCal::ComediAnalogIOSoftCal()\n");
        if (!readCalibration())
                throw "Unable to read the calibration of the DAQ board.";
}

ComediAnalogIOSoftCal::~ComediAnalogIOSoftCal()
{
        Logger(Debug, "ComediAnalogIOSoftCal::~ComediAnalogIOSoftCal()\n");
        comedi_cleanup_calibration(m_calibration);
        delete m_calibrationFile;
}

bool ComediAnalogIOSoftCal::readCalibration()
{
        Logger(Debug, "ComediAnalogIOSoftCal::readCalibration()\n");
        m_calibrationFile = comedi_get_default_calibration_path(m_device);
        if (m_calibrationFile == NULL) {
                Logger(Critical, "Unable to find a calibration file for [%s].\n", deviceFile());
                return false;
        }
        else {
                Logger(Debug, "Using calibration file [%s].\n", m_calibrationFile);
        }

        m_calibration = comedi_parse_calibration_file(m_calibrationFile);
        if (m_calibration == NULL) {
                Logger(Critical, "Unable to parse calibration file [%s].\n", m_calibrationFile);
                return false;
        }
        else {
                Logger(Debug, "Successfully parsed calibration file [%s].\n", m_calibrationFile);
        }
        return true;
}

//~~~

ComediAnalogInput::ComediAnalogInput(const char *deviceFile, uint inputSubdevice,
                                     uint readChannel, double inputConversionFactor,
                                     uint range, uint aref)
        : ComediAnalogIO(deviceFile, inputSubdevice, readChannel, range, aref),
          m_inputConversionFactor(inputConversionFactor)
{}

double ComediAnalogInput::inputConversionFactor() const
{
        return m_inputConversionFactor;
}

double ComediAnalogInput::read()
{
        lsampl_t sample;
        comedi_data_read(m_device, m_subdevice, m_channel, m_range, m_aref, &sample);
        /*
        double dt = GetGlobalDt(), now = GetGlobalTime();
        if (now > 1-dt/2 && now < 1+dt/2)
                Logger(Debug, "read: 0x%x\n", sample);
        */
        return comedi_to_phys(sample, m_dataRange, m_maxData) * m_inputConversionFactor;
}

//~~~

ComediAnalogOutput::ComediAnalogOutput(const char *deviceFile, uint outputSubdevice,
                                       uint writeChannel, double outputConversionFactor, uint aref)
        : ComediAnalogIO(deviceFile, outputSubdevice, writeChannel, PLUS_MINUS_TEN, aref),
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
        lsampl_t sample = comedi_from_phys(data*m_outputConversionFactor, m_dataRange, m_maxData);
        /*
        double dt = GetGlobalDt(), now = GetGlobalTime();
        if (now > 1-dt/2 && now < 1+dt/2)
                Logger(Debug, "written: 0x%x\n", sample);
        */
        comedi_data_write(m_device, m_subdevice, m_channel, m_range, m_aref, sample);
}

//~~~

ComediAnalogInputSoftCal::ComediAnalogInputSoftCal(const char *deviceFile, uint inputSubdevice,
                                                   uint readChannel, double inputConversionFactor,
                                                   uint range, uint aref)
        : ComediAnalogIOSoftCal(deviceFile, inputSubdevice, readChannel, range, aref),
          m_inputConversionFactor(inputConversionFactor)
{
        Logger(Debug, "ComediAnalogInputSoftCal::ComediAnalogInputSoftCal()\n");
        int flag = comedi_get_softcal_converter(m_subdevice, m_channel, m_range,
                        COMEDI_TO_PHYSICAL, m_calibration, &m_converter);
        if (flag != 0) {
                Logger(Critical, "Unable to get converter for sw-calibrated device.\n");
                throw "Error in comedi_get_softcal_converter()";
        }
}

double ComediAnalogInputSoftCal::inputConversionFactor() const
{
        return m_inputConversionFactor;
}

double ComediAnalogInputSoftCal::read()
{
        lsampl_t sample;
        comedi_data_read(m_device, m_subdevice, m_channel, m_range, m_aref, &sample);
        return comedi_to_physical(sample, &m_converter) * m_inputConversionFactor;
}

//~~~

ComediAnalogOutputSoftCal::ComediAnalogOutputSoftCal(const char *deviceFile, uint outputSubdevice,
                                                     uint writeChannel, double outputConversionFactor,
                                                     uint aref)
        : ComediAnalogIOSoftCal(deviceFile, outputSubdevice, writeChannel, PLUS_MINUS_TEN, aref),
          m_outputConversionFactor(outputConversionFactor)
{
        Logger(Debug, "ComediAnalogOutputSoftCal::ComediAnalogOutputSoftCal()\n");
        int flag = comedi_get_softcal_converter(m_subdevice, m_channel, m_range,
                        COMEDI_FROM_PHYSICAL, m_calibration, &m_converter);
        if (flag != 0) {
                Logger(Critical, "Unable to get converter for sw-calibrated device.\n");
                throw "Error in comedi_get_softcal_converter()";
        }
}

ComediAnalogOutputSoftCal::~ComediAnalogOutputSoftCal()
{
        write(0.0);
}

double ComediAnalogOutputSoftCal::outputConversionFactor() const
{
        return m_outputConversionFactor;
}

/**
 * Write the argument to the output of the DAQ board.
 */
void ComediAnalogOutputSoftCal::write(double data)
{
        comedi_data_write(m_device, m_subdevice, m_channel, m_range, m_aref,
                comedi_from_physical(data*m_outputConversionFactor, &m_converter));
}

//~~~

AnalogInput::AnalogInput(const char *deviceFile, uint inputSubdevice,
                         uint readChannel, double inputConversionFactor,
                         uint range, uint aref,
                         uint id, double dt)
        : Entity(id, dt), m_data(0.0),
          m_input(deviceFile, inputSubdevice, readChannel, inputConversionFactor, range, aref)
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
                           uint writeChannel, double outputConversionFactor, uint aref,
                           uint id, double dt)
        : Entity(id, dt),
          m_output(deviceFile, outputSubdevice, writeChannel, outputConversionFactor, aref)
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

