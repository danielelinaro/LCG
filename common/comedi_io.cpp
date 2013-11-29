#include "comedi_io.h"

#ifdef HAVE_LIBCOMEDI
#include "utils.h"
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sstream>
#include <errno.h>

namespace lcg {

char* CommandSource(int src,char *buf)
{
	buf[0]=0;
	if (src&TRIG_NONE) strcat(buf,"none|");
	if (src&TRIG_NOW) strcat(buf,"now|");
	if (src&TRIG_FOLLOW) strcat(buf, "follow|");
	if (src&TRIG_TIME) strcat(buf, "time|");
	if (src&TRIG_TIMER) strcat(buf, "timer|");
	if (src&TRIG_COUNT) strcat(buf, "count|");
	if (src&TRIG_EXT) strcat(buf, "ext|");
	if (src&TRIG_INT) strcat(buf, "int|");
#ifdef TRIG_OTHER
	if(src&TRIG_OTHER) strcat(buf, "other|");
#endif
	if(strlen(buf)==0)
		sprintf(buf, "unknown(0x%08x)", src);
        else
		buf[strlen(buf)-1] = 0;
	return buf;
}

void DumpCommand(LogLevel level, comedi_cmd *cmd)
{
	char buf[100];
	Logger(level, "subdev:         %d\n", cmd->subdev);
	Logger(level, "flags:          0x%x\n", cmd->flags);
	Logger(level, "start:          %-8s %d\n", CommandSource(cmd->start_src,buf), cmd->start_arg);
	Logger(level, "scan_begin:     %-8s %d\n", CommandSource(cmd->scan_begin_src,buf), cmd->scan_begin_arg);
	Logger(level, "convert:        %-8s %d\n", CommandSource(cmd->convert_src,buf), cmd->convert_arg);
	Logger(level, "scan_end:       %-8s %d\n", CommandSource(cmd->scan_end_src,buf), cmd->scan_end_arg);
	Logger(level, "stop:           %-8s %d\n", CommandSource(cmd->stop_src,buf), cmd->stop_arg);
	Logger(level, "chanlist_len:   %d\n", cmd->chanlist_len);
	Logger(level, "data_len:       %d\n", cmd->data_len);
}

ComediAnalogIO::ComediAnalogIO(const char *deviceFile, uint subdevice,
                               uint *channels, uint nChannels,
                               uint range, uint aref)
        : m_device(NULL), m_subdevice(subdevice), m_channels(NULL), m_nChannels(0),
          m_range(range), m_aref(aref), m_data(NULL)
{
        Logger(Debug, "ComediAnalogIO::ComediAnalogIO()\n");

        strncpy(m_deviceFile, deviceFile, 30);

        // open the device
        if (!openDevice())
                throw "Unable to open communication with the DAQ board.";

        // copy the channels to read
        if (nChannels > 0 && channels != NULL) {
                m_nChannels = nChannels;
                m_channels = new uint[m_nChannels];
                for (uint i=0; i<m_nChannels; i++)
                        m_channels[i] = channels[i];
                m_data = new lsampl_t[m_nChannels];
        }
}

ComediAnalogIO::~ComediAnalogIO()
{
        Logger(Debug, "ComediAnalogIO::~ComediAnalogIO()\n");
        closeDevice();
        if (m_nChannels > 0) {
                delete m_channels;
                delete m_data;
        }
}

bool ComediAnalogIO::openDevice()
{
        Logger(Debug, "ComediAnalogIO::openDevice()\n");
        m_device = comedi_open(m_deviceFile);
        if(m_device == NULL) {
                comedi_perror(m_deviceFile);
                return false;
        }
        return true;
}

bool ComediAnalogIO::closeDevice()
{
        if (m_device != NULL)
                return comedi_close(m_device) == 0;
        return true;
}
        
bool ComediAnalogIO::isChannelPresent(uint channel)
{
        for (uint i=0; i<m_nChannels; i++) {
                if (m_channels[i] == channel) {
                        Logger(Important, "Channel #%d already open.\n", channel);
                        return true;
                }
        }
        return false;
}

bool ComediAnalogIO::addChannel(uint channel)
{
        Logger(Debug, "ComediAnalogIO::addChannel(uint)\n");
        uint i;

        if (isChannelPresent(channel))
                return false;

        if (m_nChannels == 0) {
                m_nChannels = 1;
                m_channels = new uint[m_nChannels];
                m_channels[0] = channel;
        }
        else {
                uint *channels = new uint[m_nChannels];
                memcpy(channels, m_channels, m_nChannels*sizeof(uint));
                delete m_channels;
                m_nChannels++;
                m_channels = new uint[m_nChannels];
                memcpy(m_channels, channels, (m_nChannels-1)*sizeof(uint));
                m_channels[m_nChannels-1] = channel;
                delete channels;
                delete m_data;
        }
        m_data = new lsampl_t[m_nChannels];
        Logger(Debug, "Channel list: [");
        for (int i=0; i<m_nChannels-1; i++)
                Logger(Debug, "%d,", m_channels[i]);
        Logger(Debug, "%d].\n", m_channels[m_nChannels-1]);

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

const uint* ComediAnalogIO::channels() const
{
        return m_channels;
}

uint ComediAnalogIO::nChannels() const
{
        return m_nChannels;
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

ComediAnalogIOSoftCal::ComediAnalogIOSoftCal(const char *deviceFile, uint subdevice,
                                             uint *channels, uint nChannels,
                                             uint range, uint aref)
        : ComediAnalogIO(deviceFile, subdevice, channels, nChannels, range, aref)
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
        closeDevice();
}

bool ComediAnalogIOSoftCal::readCalibration()
{
        Logger(Debug, "ComediAnalogIOSoftCal::readCalibration()\n");
        m_calibrationFile = comedi_get_default_calibration_path(m_device);
        if (m_calibrationFile == NULL) {
                Logger(Critical, "comedi_get_default_calibration_path: %s.\n", comedi_strerror(comedi_errno()));
                return false;
        }
        else {
                Logger(Debug, "Using calibration file [%s].\n", m_calibrationFile);
        }

        m_calibration = comedi_parse_calibration_file(m_calibrationFile);
        if (m_calibration == NULL) {
                Logger(Critical, "comedi_parse_calibration_file: %s.\n", comedi_strerror(comedi_errno()));
                return false;
        }
        else {
                Logger(Debug, "Successfully parsed calibration file [%s].\n", m_calibrationFile);
        }
        return true;
}

//~~~

ComediAnalogInputSoftCal::ComediAnalogInputSoftCal(const char *deviceFile, uint inputSubdevice,
                                                   uint readChannel, double inputConversionFactor,
                                                   uint range, uint aref)
        : ComediAnalogIOSoftCal(deviceFile, inputSubdevice, &readChannel, 1, range, aref),
          m_inputConversionFactor(inputConversionFactor)
{
        Logger(Debug, "ComediAnalogInputSoftCal::ComediAnalogInputSoftCal()\n");
        int flag = comedi_get_softcal_converter(m_subdevice, m_channels[0], m_range,
                        COMEDI_TO_PHYSICAL, m_calibration, &m_converter);
        if (flag != 0) {
                Logger(Critical, "comedi_get_softcal_converter: %s.\n", comedi_strerror(comedi_errno()));
                throw "Error in comedi_get_softcal_converter()";
        }
}

ComediAnalogInputSoftCal::~ComediAnalogInputSoftCal()
{
}

bool ComediAnalogInputSoftCal::initialise()
{
        return true;
}

double ComediAnalogInputSoftCal::inputConversionFactor() const
{
        return m_inputConversionFactor;
}

double ComediAnalogInputSoftCal::read()
{
        lsampl_t sample;
        comedi_data_read(m_device, m_subdevice, m_channels[0], m_range, m_aref, &sample);
        return comedi_to_physical(sample, &m_converter) * m_inputConversionFactor;
}

//~~~

ComediAnalogOutputSoftCal::ComediAnalogOutputSoftCal(const char *deviceFile, uint outputSubdevice,
                                                     uint writeChannel, double outputConversionFactor,
                                                     uint aref)
        : ComediAnalogIOSoftCal(deviceFile, outputSubdevice, &writeChannel, 1, PLUS_MINUS_TEN, aref),
          m_outputConversionFactor(outputConversionFactor)
{
        Logger(Debug, "ComediAnalogOutputSoftCal::ComediAnalogOutputSoftCal()\n");
        int flag = comedi_get_softcal_converter(m_subdevice, m_channels[0], m_range,
                        COMEDI_FROM_PHYSICAL, m_calibration, &m_converter);
        if (flag != 0) {
                Logger(Critical, "comedi_get_softcal_converter: %s.\n", comedi_strerror(comedi_errno()));
                throw "Error in comedi_get_softcal_converter()";
        }
#ifdef TRIM_ANALOG_OUTPUT
        // get physical data range for subdevice (min, max, phys. units)
        m_dataRange = comedi_get_range(m_device, m_subdevice, m_channels[0], m_range);
        Logger(Debug, "Range for 0x%x (%s):\n\tmin = %g\n\tmax = %g\n", m_dataRange,
                        (m_dataRange->unit == UNIT_volt ? "volts" : "milliamps"),
                        m_dataRange->min, m_dataRange->max);
        if(m_dataRange == NULL) {
                Logger(Critical, "comedi_get_range: %s.\n", comedi_strerror(comedi_errno()));
                throw "Error in comedi_get_range()";
        }
#endif
}

ComediAnalogOutputSoftCal::~ComediAnalogOutputSoftCal()
{
#ifdef RESET_OUTPUT
        write(0.0);
#endif
}

bool ComediAnalogOutputSoftCal::initialise()
{
#ifdef RESET_OUTPUT
        write(0.0);
#endif
        return true;
}

double ComediAnalogOutputSoftCal::outputConversionFactor() const
{
        return m_outputConversionFactor;
}

void ComediAnalogOutputSoftCal::write(double data)
{
	double sample = data*m_outputConversionFactor;
#ifdef TRIM_ANALOG_OUTPUT
	if (sample <= m_dataRange->min) {
		sample = m_dataRange->min + 0.01*(m_dataRange->max-m_dataRange->min);
		//Logger(Debug, "[%f] - Trimming lower limit of the DAQ card.\n", GetGlobalTime());
	}
	if (sample >= m_dataRange->max) {
		sample = m_dataRange->max - 0.01*(m_dataRange->max-m_dataRange->min);
		//Logger(Debug, "[%f] - Trimming upper limit of the DAQ card.\n", GetGlobalTime());
	}
#endif
        comedi_data_write(m_device, m_subdevice, m_channels[0], m_range, m_aref,
                comedi_from_physical(sample, &m_converter));
}

//~~~

ComediAnalogInputHardCal::ComediAnalogInputHardCal(const char *deviceFile, uint inputSubdevice,
                                                   uint readChannel, double inputConversionFactor,
                                                   uint range, uint aref)
        : ComediAnalogIO(deviceFile, inputSubdevice, &readChannel, 1, range, aref),
          m_inputConversionFactor(inputConversionFactor)
{
        initialise();
}

bool ComediAnalogInputHardCal::initialise()
{
        // get physical data range for subdevice (min, max, phys. units)
        m_dataRange = comedi_get_range(m_device, m_subdevice, m_channels[0], m_range);
        Logger(Debug, "Range for 0x%x (%s):\n\tmin = %g\n\tmax = %g\n", m_dataRange,
                        (m_dataRange->unit == UNIT_volt ? "volts" : "milliamps"),
                        m_dataRange->min, m_dataRange->max);
        if(m_dataRange == NULL) {
                comedi_perror(m_deviceFile);
                comedi_close(m_device);
                return false;
        }
        
        // read max data value
        m_maxData = comedi_get_maxdata(m_device, m_subdevice, m_channels[0]);
        Logger(Debug, "Max data = %ld\n", m_maxData);
}

double ComediAnalogInputHardCal::inputConversionFactor() const
{
        return m_inputConversionFactor;
}

double ComediAnalogInputHardCal::read()
{
        lsampl_t sample;
        comedi_data_read(m_device, m_subdevice, m_channels[0], m_range, m_aref, &sample);
        return comedi_to_phys(sample, m_dataRange, m_maxData) * m_inputConversionFactor;
}

//~~~

ComediAnalogOutputHardCal::ComediAnalogOutputHardCal(const char *deviceFile, uint outputSubdevice,
                                                     uint writeChannel, double outputConversionFactor, uint aref)
        : ComediAnalogIO(deviceFile, outputSubdevice, &writeChannel, 1, PLUS_MINUS_TEN, aref),
          m_outputConversionFactor(outputConversionFactor)
{
        initialise();
#ifdef RESET_OUTPUT
        write(0.0);
#endif
}

ComediAnalogOutputHardCal::~ComediAnalogOutputHardCal()
{
#ifdef RESET_OUTPUT
        write(0.0);
#endif
}

bool ComediAnalogOutputHardCal::initialise()
{
        // get physical data range for subdevice (min, max, phys. units)
        m_dataRange = comedi_get_range(m_device, m_subdevice, m_channels[0], m_range);
        Logger(Debug, "Range for 0x%x (%s):\n\tmin = %g\n\tmax = %g\n", m_dataRange,
                        (m_dataRange->unit == UNIT_volt ? "volts" : "milliamps"),
                        m_dataRange->min, m_dataRange->max);
        if(m_dataRange == NULL) {
                comedi_perror(m_deviceFile);
                comedi_close(m_device);
                return false;
        }
        
        // read max data value
        m_maxData = comedi_get_maxdata(m_device, m_subdevice, m_channels[0]);
        Logger(Debug, "Max data = %ld\n", m_maxData);
}

double ComediAnalogOutputHardCal::outputConversionFactor() const
{
        return m_outputConversionFactor;
}

void ComediAnalogOutputHardCal::write(double data)
{
        lsampl_t sample = comedi_from_phys(data*m_outputConversionFactor, m_dataRange, m_maxData);
        comedi_data_write(m_device, m_subdevice, m_channels[0], m_range, m_aref, sample);
}

} // namespace lcg

#endif // HAVE_LIBCOMEDI

