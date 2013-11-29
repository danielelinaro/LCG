#include "analogy_io.h"

#include "engine.h"
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sstream>

namespace lcg {

AnalogyAnalogIO::AnalogyAnalogIO(const char *deviceFile, uint subdevice,
                               uint *channels, uint nChannels,
                               uint range, uint aref)
        : m_subdevice(subdevice), m_channels(NULL), m_nChannels(0),
          m_range(range), m_aref(aref), m_data(NULL)
{
        Logger(Debug, "AnalogyAnalogIO::AnalogyAnalogIO()\n");

        strncpy(m_deviceFile, deviceFile, 30);

        // copy the channels to read
        if (channels != NULL && nChannels > 0) {
                m_nChannels = nChannels;
                m_channels = new uint[m_nChannels];
                for (uint i=0; i<m_nChannels; i++)
                        m_channels[i] = channels[i];
                m_data = new double[m_nChannels];
        }
        
        // open the device
        if (!openDevice())
                throw "Unable to open communication with the DAQ board.";
}

AnalogyAnalogIO::~AnalogyAnalogIO()
{
        Logger(Debug, "AnalogyAnalogIO::~AnalogyAnalogIO()\n");
        closeDevice();
        if (m_nChannels > 0) {
                delete m_channels;
                delete m_data;
        }
}

bool AnalogyAnalogIO::openDevice()
{
        int flag, i;

        Logger(Debug, "AnalogyAnalogIO::openDevice()\n");

        flag = a4l_open(&m_dsc, m_deviceFile);
        if (flag != 0) {
                Logger(Critical, "Unable to open device [%s].\n", m_deviceFile);
                return false;
        }

	/* Allocate a buffer so as to get more info (subd, chan, rng) */
	m_dsc.sbdata = malloc(m_dsc.sbsize);
	if (m_dsc.sbdata == NULL) {
		Logger(Critical, "Unable to allocate buffer.\n");
		return false;
	}

	/* Get this data */
	flag = a4l_fill_desc(&m_dsc);
	if (flag < 0) {
		Logger(Critical, "AnalogyAnalogIO::openDevice(): a4l_fill_desc failed (flag=%d)\n", flag);
		return false;
	}

	/* Get the size of a single acquisition */
        /*
        int scan_size = 0;
	for (i=0; i<m_nChannels; i++) {
	        a4l_chinfo_t *chinfo;
                a4l_rnginfo_t *rnginfo;

		flag = a4l_get_chinfo(&m_dsc, m_subdevice, m_channels[i], &chinfo);
		if (flag < 0) {
			Logger(Critical, "AnalogyAnalogIO::openDevice(): a4l_get_chinfo failed (flag=%d)\n", flag);
			return false;
		}

		flag = a4l_get_rnginfo(&m_dsc, m_subdevice, m_channels[i], &rnginfo);
		if (flag < 0) {
			Logger(Critical, "AnalogyAnalogIO::openDevice(): a4l_get_rnginfo failed (flag=%d)\n", flag);
			return false;
		}

		Logger(Info, "AnalogyAnalogIO::openDevice(): channel %x\n", m_channels[i]);
		Logger(Info, "\t ranges count = %d\n", chinfo->nb_rng);
		Logger(Info, "\t bit width = %d (bits)\n", chinfo->nb_bits);

		scan_size += a4l_sizeof_chan(chinfo);
	}
	Logger(Info, "AnalogyAnalogIO::openDevice(): scan size = %u\n", scan_size);
        */

        //if (cmd.stop_arg != 0)
        //	printf("cmd_read: size to read = %u\n", scan_size * cmd.stop_arg);

	/* Cancel any former command which might be in progress */
	a4l_snd_cancel(&m_dsc, m_subdevice);

        return true;
}

bool AnalogyAnalogIO::closeDevice()
{
        return a4l_close(&m_dsc) == 0;
}
        
bool AnalogyAnalogIO::isChannelPresent(uint channel)
{
        for (uint i=0; i<m_nChannels; i++) {
                if (m_channels[i] == channel) {
                        Logger(Important, "Channel #%d already open.\n", channel);
                        return true;
                }
        }
        return false;
}

bool AnalogyAnalogIO::addChannel(uint channel)
{
        Logger(Info, "AnalogyAnalogIO::addChannel(uint)\n");
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
        m_data = new double[m_nChannels];

        return true;
}

const char* AnalogyAnalogIO::deviceFile() const
{
        return m_deviceFile;
}

uint AnalogyAnalogIO::subdevice() const
{
        return m_subdevice;
}

const uint* AnalogyAnalogIO::channels() const
{
        return m_channels;
}

uint AnalogyAnalogIO::nChannels() const
{
        return m_nChannels;
}

uint AnalogyAnalogIO::range() const
{
        return m_range;
}

uint AnalogyAnalogIO::reference() const
{
        return m_aref;
}

//~~~

/*
AnalogyAnalogInputHardCal::AnalogyAnalogInputHardCal(const char *deviceFile, uint inputSubdevice,
                                                   uint readChannel, double inputConversionFactor,
                                                   uint range, uint aref)
        : AnalogyAnalogIO(deviceFile, inputSubdevice, &readChannel, 1, range, aref),
          m_inputConversionFactor(inputConversionFactor)
{
        initialise();
}

bool AnalogyAnalogInputHardCal::initialise()
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

double AnalogyAnalogInputHardCal::inputConversionFactor() const
{
        return m_inputConversionFactor;
}

double AnalogyAnalogInputHardCal::read()
{
        lsampl_t sample;
        comedi_data_read(m_device, m_subdevice, m_channels[0], m_range, m_aref, &sample);
        return comedi_to_phys(sample, m_dataRange, m_maxData) * m_inputConversionFactor;
}

//~~~

AnalogyAnalogOutputHardCal::AnalogyAnalogOutputHardCal(const char *deviceFile, uint outputSubdevice,
                                                     uint writeChannel, double outputConversionFactor, uint aref)
        : AnalogyAnalogIO(deviceFile, outputSubdevice, &writeChannel, 1, PLUS_MINUS_TEN, aref),
          m_outputConversionFactor(outputConversionFactor)
{
        initialise();
#ifdef RESET_OUTPUT
        write(0.0);
#endif
}

AnalogyAnalogOutputHardCal::~AnalogyAnalogOutputHardCal()
{
#ifdef RESET_OUTPUT
        write(0.0);
#endif
}

bool AnalogyAnalogOutputHardCal::initialise()
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

double AnalogyAnalogOutputHardCal::outputConversionFactor() const
{
        return m_outputConversionFactor;
}

// Write the argument to the output of the DAQ board.
void AnalogyAnalogOutputHardCal::write(double data)
{
        lsampl_t sample = comedi_from_phys(data*m_outputConversionFactor, m_dataRange, m_maxData);
        comedi_data_write(m_device, m_subdevice, m_channels[0], m_range, m_aref, sample);
}

*/
//~~~

AnalogyAnalogIOSoftCal::AnalogyAnalogIOSoftCal(const char *deviceFile, uint subdevice,
                                             uint *channels, uint nChannels,
                                             uint range, uint aref)
        : AnalogyAnalogIO(deviceFile, subdevice, channels, nChannels, range, aref)
{
        Logger(Debug, "AnalogyAnalogIOSoftCal::AnalogyAnalogIOSoftCal()\n");
        if (!readCalibration())
                throw "Unable to read the calibration of the DAQ board.";
}

AnalogyAnalogIOSoftCal::~AnalogyAnalogIOSoftCal()
{
        Logger(Debug, "AnalogyAnalogIOSoftCal::~AnalogyAnalogIOSoftCal()\n");
        comedi_cleanup_calibration(m_calibration);
        delete m_calibrationFile;
        closeDevice();
}

bool AnalogyAnalogIOSoftCal::readCalibration()
{
        Logger(Debug, "AnalogyAnalogIOSoftCal::readCalibration()\n");
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

AnalogyAnalogInputSoftCal::AnalogyAnalogInputSoftCal(const char *deviceFile, uint inputSubdevice,
                                                   uint readChannel, double inputConversionFactor,
                                                   uint range, uint aref)
        : AnalogyAnalogIOSoftCal(deviceFile, inputSubdevice, &readChannel, 1, range, aref),
          m_inputConversionFactor(inputConversionFactor)
{
        Logger(Debug, "AnalogyAnalogInputSoftCal::AnalogyAnalogInputSoftCal()\n");
        int flag = comedi_get_softcal_converter(m_subdevice, m_channels[0], m_range,
                        COMEDI_TO_PHYSICAL, m_calibration, &m_converter);
        if (flag != 0) {
                Logger(Critical, "Unable to get converter for sw-calibrated device.\n");
                throw "Error in comedi_get_softcal_converter()";
        }
}

AnalogyAnalogInputSoftCal::~AnalogyAnalogInputSoftCal()
{
}

bool AnalogyAnalogInputSoftCal::initialise()
{
        return true;
}

double AnalogyAnalogInputSoftCal::inputConversionFactor() const
{
        return m_inputConversionFactor;
}

double AnalogyAnalogInputSoftCal::read()
{
        lsampl_t sample;
        comedi_data_read(m_device, m_subdevice, m_channels[0], m_range, m_aref, &sample);
        return comedi_to_physical(sample, &m_converter) * m_inputConversionFactor;
}

//~~~

AnalogyAnalogOutputSoftCal::AnalogyAnalogOutputSoftCal(const char *deviceFile, uint outputSubdevice,
                                                     uint writeChannel, double outputConversionFactor,
                                                     uint aref)
        : AnalogyAnalogIOSoftCal(deviceFile, outputSubdevice, &writeChannel, 1, PLUS_MINUS_TEN, aref),
          m_outputConversionFactor(outputConversionFactor)
{
        Logger(Debug, "AnalogyAnalogOutputSoftCal::AnalogyAnalogOutputSoftCal()\n");
        int flag = comedi_get_softcal_converter(m_subdevice, m_channels[0], m_range,
                        COMEDI_FROM_PHYSICAL, m_calibration, &m_converter);
        if (flag != 0) {
                Logger(Critical, "Unable to get converter for sw-calibrated device.\n");
                throw "Error in comedi_get_softcal_converter()";
        }
}

AnalogyAnalogOutputSoftCal::~AnalogyAnalogOutputSoftCal()
{
#ifdef RESET_OUTPUT
        write(0.0);
#endif
}

bool AnalogyAnalogOutputSoftCal::initialise()
{
#ifdef RESET_OUTPUT
        write(0.0);
#endif
        return true;
}

double AnalogyAnalogOutputSoftCal::outputConversionFactor() const
{
        return m_outputConversionFactor;
}

// Write the argument to the output of the DAQ board.
void AnalogyAnalogOutputSoftCal::write(double data)
{
        comedi_data_write(m_device, m_subdevice, m_channels[0], m_range, m_aref,
                comedi_from_physical(data*m_outputConversionFactor, &m_converter));
}

} // namespace lcg



