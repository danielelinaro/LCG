#include "comedi_io.h"

#ifdef HAVE_LIBCOMEDI
#include "engine.h"
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sstream>

namespace dynclamp {

#ifdef ASYNCHRONOUS_INPUT
std::map<std::string,ComediAnalogInputProxy*> proxies;
#endif

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
        Logger(Info, "Channel list: [");
        for (int i=0; i<m_nChannels-1; i++)
                Logger(Info, "%d,", m_channels[i]);
        Logger(Info, "%d].\n", m_channels[m_nChannels-1]);

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
	Logger(level, "flags:          %x\n", cmd->flags);
	Logger(level, "start:          %-8s %d\n", CommandSource(cmd->start_src,buf), cmd->start_arg);
	Logger(level, "scan_begin:     %-8s %d\n", CommandSource(cmd->scan_begin_src,buf), cmd->scan_begin_arg);
	Logger(level, "convert:        %-8s %d\n", CommandSource(cmd->convert_src,buf), cmd->convert_arg);
	Logger(level, "scan_end:       %-8s %d\n", CommandSource(cmd->scan_end_src,buf), cmd->scan_end_arg);
	Logger(level, "stop:           %-8s %d\n", CommandSource(cmd->stop_src,buf), cmd->stop_arg);
	Logger(level, "chanlist_len:   %d\n", cmd->chanlist_len);
	Logger(level, "data_len:       %d\n", cmd->data_len);
}

ComediAnalogInputProxy::ComediAnalogInputProxy(const char *deviceFile, uint subdevice,
                                               uint *channels, uint nChannels,
                                               uint range, uint aref)
        : ComediAnalogIO(deviceFile, subdevice, channels, nChannels, range, aref),
          m_commandRunning(false), m_tLastSample(-1.), m_refCount(0),
          m_channelsPacked(NULL), m_hash(), m_bytesToRead(0)
{
        m_subdeviceFlags = comedi_get_subdevice_flags(m_device, m_subdevice);
        if (m_subdeviceFlags & SDF_LSAMPL)
                m_bytesPerSample = sizeof(lsampl_t);
        else
                m_bytesPerSample = sizeof(sampl_t);
        m_deviceFd = comedi_fileno(m_device);
        Logger(Debug, "Sample size is %d bytes.\n", m_bytesPerSample);
}

ComediAnalogInputProxy::~ComediAnalogInputProxy()
{
        Logger(Debug, "ComediAnalogInputProxy::~ComediAnalogInputProxy()\n");
        if (m_channelsPacked != NULL)
                delete m_channelsPacked;
        stopCommand();
}

void ComediAnalogInputProxy::increaseRefCount()
{
        m_refCount++;
}

void ComediAnalogInputProxy::decreaseRefCount()
{
        m_refCount--;
        if (m_refCount == 0)
                delete this;
}

uint ComediAnalogInputProxy::refCount() const
{
        return m_refCount;
}

bool ComediAnalogInputProxy::initialise()
{
        Logger(Debug, "ComediAnalogInputProxy::initialise()\n");
        if (m_nChannels == 0) {
                Logger(Critical, "No channels are configured.\n");
                return false;
        }

        stopCommand();

        packChannelsList();

        // the command doesn't start immediately
        bool flag = issueCommand();
        if (flag) {
                Logger(Debug, "Successfully issued the command.\n");
                flag = startCommand();
                if (flag)
                        Logger(Debug, "Successfully started the acquisition.\n");
                else
                        Logger(Critical, "Unable to start the acquisition.\n");
        }
        else {
                Logger(Critical, "Unable to issue the command.\n");
        }

        return flag;
}

bool ComediAnalogInputProxy::stopCommand()
{
        if (!m_commandRunning) {
                Logger(Debug, "The command is not running.\n");
                return true;
        }

        if (comedi_cancel(m_device, m_subdevice) != 0) {
                Logger(Critical, "Error stopping the command.\n");
                return false;
        }

        Logger(Debug, "Successfully stopped running command.\n");

        // read data that may have remained in the internal buffer of the card
        char buffer[1024];
        size_t n;
        while ((n = read(m_deviceFd, buffer, 1024)) > 0)
                Logger(Info, "Read %d remaining bytes.\n");

        m_commandRunning = false;
        return true;
}

void ComediAnalogInputProxy::packChannelsList()
{
        if (m_channelsPacked != NULL)
                delete m_channelsPacked;
        m_channelsPacked = new uint[m_nChannels];
        for (uint i=0; i<m_nChannels; i++) {
                m_channelsPacked[i] = CR_PACK(m_channels[i], m_range, m_aref);
                Logger(Debug, "Packed channel %d.\n", m_channels[i]);
        }
}


bool ComediAnalogInputProxy::issueCommand()
{
        memset(&m_cmd, 0, sizeof(struct comedi_cmd_struct));
        int ret = comedi_get_cmd_generic_timed(m_device, m_subdevice, &m_cmd,
                        m_nChannels, int(NSEC_PER_SEC * GetGlobalDt())/OVERSAMPLING_FACTOR);
        if (ret < 0) {
                Logger(Critical, "Error in comedi_get_cmd_generic_timed.\n");
                return false;
        }
        m_cmd.convert_arg = 0;    // this value is wrong, but will be fixed by comedi_command_test
        m_cmd.chanlist    = m_channelsPacked;
        m_cmd.start_src   = TRIG_INT;
        m_cmd.stop_src    = TRIG_COUNT;
        m_cmd.stop_arg    = (int) ceil(GetRunTime()/GetGlobalDt())*OVERSAMPLING_FACTOR;

        m_bytesToRead = m_nChannels * m_bytesPerSample * OVERSAMPLING_FACTOR;

        if (!fixCommand())
                return false;

        if (comedi_command(m_device, &m_cmd) != 0) {
                Logger(Critical, "Error starting the command.\n");
                return false;
        }

        return true;
}

bool ComediAnalogInputProxy::fixCommand()
{                
        if (comedi_command_test(m_device, &m_cmd) < 0 && 
            comedi_command_test(m_device, &m_cmd) < 0) {
                Logger(Critical, "Invalid command.\n");
                return false;
        }
        Logger(Debug, "Successfully fixed the command.\n");
        Logger(Debug, "--------------------------------\n");
        DumpCommand(Debug, &m_cmd);
        Logger(Debug, "--------------------------------\n");
        return true;
}

bool ComediAnalogInputProxy::startCommand()
{
        if (m_commandRunning) {
                Logger(Info, "The command is already running.\n");
                return true;
        }
        
        comedi_insn insn;
        unsigned int data = 0;

        // prepare the triggering instruction
        memset(&insn, 0, sizeof(insn));
        insn.insn = INSN_INTTRIG;
        insn.n = 1;
        insn.data = &data;
        insn.subdev = m_subdevice;

        // start the acquisition
        if (comedi_do_insn(m_device, &insn) < 0) {
                Logger(Critical, "Unable to start the acquisition (error in comedi_do_insn).\n");
                return false;
        }

        m_commandRunning = true;

        return true;
}

void ComediAnalogInputProxy::acquire()
{
        if (!m_commandRunning)
                startCommand();
        double now = GetGlobalTime();
        if (now != m_tLastSample) {

                // read the data
                size_t nBytes = 0;
                char *ptr = m_buffer;
                do {
                        nBytes += read(m_deviceFd, ptr, m_bytesToRead-nBytes);
                        ptr += nBytes;
                } while (nBytes != 0 && nBytes < m_bytesToRead);

                for (int i=0; i<m_nChannels; i++) {
                        if (m_subdeviceFlags & SDF_LSAMPL)
                                m_data[i] = ((lsampl_t *) m_buffer)[i];
                        else
                                m_data[i] = ((sampl_t *) m_buffer)[i];
                        m_hash[m_channels[i]] = m_data[i];
                }

                // update the time of last read operation
                m_tLastSample = now;
        }
}

lsampl_t ComediAnalogInputProxy::value(uint channel)
{
        return m_hash[channel];
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

/**
 * Write the argument to the output of the DAQ board.
 */
void ComediAnalogOutputHardCal::write(double data)
{
        lsampl_t sample = comedi_from_phys(data*m_outputConversionFactor, m_dataRange, m_maxData);
        comedi_data_write(m_device, m_subdevice, m_channels[0], m_range, m_aref, sample);
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
                Logger(Critical, "Unable to get converter for sw-calibrated device.\n");
                throw "Error in comedi_get_softcal_converter()";
        }

#ifdef ASYNCHRONOUS_INPUT
        std::stringstream key;
        key << deviceFile << "-" << inputSubdevice;
        if (proxies.count(key.str()) == 0) {
                Logger(Info, "Adding proxy for device %s and subdevice %d.\n", m_deviceFile, inputSubdevice);
                proxies[key.str()] = new ComediAnalogInputProxy(deviceFile, inputSubdevice, &readChannel, 1, range, aref);
        }
        else {
                proxies[key.str()]->addChannel(m_channels[0]);
        }
        m_proxy = proxies[key.str()];
        m_proxy->increaseRefCount();
#endif
}

ComediAnalogInputSoftCal::~ComediAnalogInputSoftCal()
{
#ifdef ASYNCHRONOUS_INPUT
        m_proxy->decreaseRefCount();
#endif
}

bool ComediAnalogInputSoftCal::initialise()
{
#ifndef ASYNCHRONOUS_INPUT
        return true;
#else
        return m_proxy->initialise();
#endif
}

double ComediAnalogInputSoftCal::inputConversionFactor() const
{
        return m_inputConversionFactor;
}

double ComediAnalogInputSoftCal::read()
{
#ifndef ASYNCHRONOUS_INPUT
        lsampl_t sample;
        comedi_data_read(m_device, m_subdevice, m_channels[0], m_range, m_aref, &sample);
        return comedi_to_physical(sample, &m_converter) * m_inputConversionFactor;
#else
        m_proxy->acquire();
        return comedi_to_physical(m_proxy->value(m_channels[0]), &m_converter) * m_inputConversionFactor;
#endif
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
                Logger(Critical, "Unable to get converter for sw-calibrated device.\n");
                throw "Error in comedi_get_softcal_converter()";
        }
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

/**
 * Write the argument to the output of the DAQ board.
 */
void ComediAnalogOutputSoftCal::write(double data)
{
        comedi_data_write(m_device, m_subdevice, m_channels[0], m_range, m_aref,
                comedi_from_physical(data*m_outputConversionFactor, &m_converter));
}

} // namespace dynclamp

#endif // HAVE_LIBCOMEDI



