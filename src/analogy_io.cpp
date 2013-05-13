#include "analogy_io.h"

#ifdef HAVE_LIBANALOGY
#include "engine.h"
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sstream>

namespace lcg {

#ifdef ASYNCHRONOUS_INPUT
//std::map<std::string,AnalogyAnalogInputProxy*> proxies;
#endif

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

//char *cmd_src(int src,char *buf)
//{
//	buf[0]=0;
//
//	if(src&TRIG_NONE)strcat(buf,"none|");
//	if(src&TRIG_NOW)strcat(buf,"now|");
//	if(src&TRIG_FOLLOW)strcat(buf, "follow|");
//	if(src&TRIG_TIME)strcat(buf, "time|");
//	if(src&TRIG_TIMER)strcat(buf, "timer|");
//	if(src&TRIG_COUNT)strcat(buf, "count|");
//	if(src&TRIG_EXT)strcat(buf, "ext|");
//	if(src&TRIG_INT)strcat(buf, "int|");
//#ifdef TRIG_OTHER
//	if(src&TRIG_OTHER)strcat(buf, "other|");
//#endif
//
//	if(strlen(buf)==0){
//		sprintf(buf,"unknown(0x%08x)",src);
//	}else{
//		buf[strlen(buf)-1]=0;
//	}
//
//	return buf;
//}
//
//void dump_cmd(FILE *out,comedi_cmd *cmd)
//{
//	char buf[100];
//
//	fprintf(out,"subdev:      %d\n", cmd->subdev);
//	fprintf(out,"flags:       %x\n", cmd->flags);
//
//	fprintf(out,"start:      %-8s %d\n",
//		cmd_src(cmd->start_src,buf),
//		cmd->start_arg);
//
//	fprintf(out,"scan_begin: %-8s %d\n",
//		cmd_src(cmd->scan_begin_src,buf),
//		cmd->scan_begin_arg);
//
//	fprintf(out,"convert:    %-8s %d\n",
//		cmd_src(cmd->convert_src,buf),
//		cmd->convert_arg);
//
//	fprintf(out,"scan_end:   %-8s %d\n",
//		cmd_src(cmd->scan_end_src,buf),
//		cmd->scan_end_arg);
//
//	fprintf(out,"stop:       %-8s %d\n",
//		cmd_src(cmd->stop_src,buf),
//		cmd->stop_arg);
//	
//	fprintf(out,"chanlist_len: %d\n", cmd->chanlist_len);
//	fprintf(out,"data_len: %d\n", cmd->data_len);
//	
//}
//
//AnalogyAnalogInputProxy::AnalogyAnalogInputProxy(const char *deviceFile, uint subdevice,
//                                               uint *channels, uint nChannels,
//                                               uint range, uint aref)
//        : AnalogyAnalogIO(deviceFile, subdevice, channels, nChannels, range, aref),
//          m_commandRunning(false), m_tLastSample(-1.), m_refCount(0),
//          m_channelsPacked(NULL), m_hash(), m_bytesToRead(0)
//{
//        m_subdeviceFlags = comedi_get_subdevice_flags(m_device, m_subdevice);
//        if (m_subdeviceFlags & SDF_LSAMPL)
//                m_bytesPerSample = sizeof(lsampl_t);
//        else
//                m_bytesPerSample = sizeof(sampl_t);
//        m_deviceFd = comedi_fileno(m_device);
//        Logger(Important, "Sample size is %d bytes.\n", m_bytesPerSample);
//}
//
//AnalogyAnalogInputProxy::~AnalogyAnalogInputProxy()
//{
//        Logger(Debug, "AnalogyAnalogInputProxy::~AnalogyAnalogInputProxy()\n");
//        if (m_channelsPacked != NULL)
//                delete m_channelsPacked;
//        stopCommand();
//}
//
//void AnalogyAnalogInputProxy::increaseRefCount()
//{
//        m_refCount++;
//}
//
//void AnalogyAnalogInputProxy::decreaseRefCount()
//{
//        m_refCount--;
//        if (m_refCount == 0)
//                delete this;
//}
//
//uint AnalogyAnalogInputProxy::refCount() const
//{
//        return m_refCount;
//}
//
//bool AnalogyAnalogInputProxy::fixCommand()
//{                
//        if (comedi_command_test(m_device, &m_cmd) < 0 && 
//            comedi_command_test(m_device, &m_cmd) < 0) {
//                Logger(Critical, "Invalid command.");
//                return false;
//        }
//        Logger(Info, "Successfully fixed the command.\n");
//        Logger(Info, "--------------------------------\n");
//        dump_cmd(stderr, &m_cmd);
//        Logger(Info, "--------------------------------\n");
//        return true;
//}
//
//bool AnalogyAnalogInputProxy::initialise()
//{
//        if (m_nChannels == 0) {
//                Logger(Critical, "No channels are configured.\n");
//                return false;
//        }
//
//        Logger(Important, "Initialising AnalogyAnalogInputProxy.\n");
//
//        stopCommand();
//
//        packChannelsList();
//
//        memset(&m_cmd, 0, sizeof(struct comedi_cmd_struct));
//        int ret = comedi_get_cmd_generic_timed(m_device, m_subdevice, &m_cmd, m_nChannels, int(1e9 * GetGlobalDt()));
//        if (ret < 0) {
//                Logger(Critical, "Error in comedi_get_cmd_generic_timed.\n");
//                return false;
//        }
//        //m_cmd.flags            |= TRIG_RT;
//        m_cmd.convert_arg       = 0;    // this value is wrong, but will be fixed by comedi_command_test
//        m_cmd.chanlist          = m_channelsPacked;
//        m_cmd.stop_src          = TRIG_NONE;
//        m_cmd.stop_arg          = 0;
//
//        /*
//        m_cmd.subdev = m_subdevice;
//        m_cmd.flags = 0;
//        //m_cmd.flags |= TRIG_RT;
//        m_cmd.start_src = TRIG_NOW;
//        m_cmd.start_arg = 0;
//        m_cmd.scan_begin_src = TRIG_TIMER;
//        m_cmd.scan_begin_arg = (uint) (GetGlobalDt()*1000000000); // period in nanoseconds
//        m_cmd.convert_src = TRIG_TIMER;
//        m_cmd.convert_arg = 0; // will be adjusted by comedi_command_test
//        m_cmd.scan_end_src = TRIG_COUNT;
//        m_cmd.scan_end_arg = m_nChannels;
//        m_cmd.stop_src = TRIG_NONE;
//        m_cmd.stop_arg = 0;
//        m_cmd.chanlist = m_channelsPacked;
//        m_cmd.chanlist_len = m_nChannels;
//        */
//
//        m_bytesToRead = m_nChannels * m_bytesPerSample;
//
//        return fixCommand() && startCommand();
//}
//
//void AnalogyAnalogInputProxy::packChannelsList()
//{
//        if (m_channelsPacked != NULL)
//                delete m_channelsPacked;
//        m_channelsPacked = new uint[m_nChannels];
//        for (uint i=0; i<m_nChannels; i++) {
//                m_channelsPacked[i] = CR_PACK(m_channels[i], m_range, m_aref);
//                Logger(Debug, "Packed channel %d.\n", m_channels[i]);
//        }
//}
//
//bool AnalogyAnalogInputProxy::stopCommand()
//{
//        if (!m_commandRunning) {
//                Logger(Info, "The command is not running.\n");
//                return true;
//        }
//
//        if (comedi_cancel(m_device, m_subdevice) != 0) {
//                Logger(Critical, "Error stopping the command.\n");
//                return false;
//        }
//
//        Logger(Info, "Successfully stopped running command.\n");
//
//        char buffer[1024];
//        size_t n;
//        while ((n = read(m_deviceFd, buffer, 1024)) > 0)
//                Logger(Info, "Read %d bytes.\n");
//
//        m_commandRunning = false;
//        return true;
//}
//
//bool AnalogyAnalogInputProxy::startCommand()
//{
//        if (m_commandRunning) {
//                Logger(Info, "The command is already running.\n");
//                return true;
//        }
//
//        if (comedi_command(m_device, &m_cmd) != 0) {
//                Logger(Critical, "Error starting the command.\n");
//                return false;
//        }
//
//        m_commandRunning = true;
//        return true;
//}
//
///*
//bool AnalogyAnalogInputProxy::addChannel(uint channel)
//{
//        Logger(Info, "AnalogyAnalogInputProxy::addChannel(uint)\n");
//
//        if (! AnalogyAnalogIO::addChannel(channel))
//                return false;
//
//        delete m_channelsPacked;
//        m_channelsPacked = new uint[m_nChannels];
//        for (uint i=0; i<m_nChannels; i++) {
//                m_channelsPacked[i] = CR_PACK(m_channels[i], m_range, m_aref);
//                Logger(Info, "Packed channel %d.\n", m_channels[i]);
//        }
//        m_cmd.chanlist = m_channelsPacked;
//        m_cmd.chanlist_len = m_nChannels;
//        m_cmd.scan_end_arg = m_nChannels;
//        fixCommand();
//
//        return true;
//}
//*/
//
//void AnalogyAnalogInputProxy::acquire()
//{
//        char buffer[512];
//        size_t nBytes;
//        int i;
//        double now = GetGlobalTime();
//        if (now != m_tLastSample) {
//                // read the actual data
//                nBytes = read(m_deviceFd, buffer, m_bytesToRead);
//
//                for (i=0; i<nBytes/m_bytesPerSample; i++) {
//                        if (m_subdeviceFlags & SDF_LSAMPL)
//                                m_data[i] = ((lsampl_t *) buffer)[i];
//                        else
//                                m_data[i] = ((sampl_t *) buffer)[i];
//                }
//
//                // copy the samples into the data hashtable
//                for (i=0; i<m_nChannels; i++)
//                        m_hash[m_channels[i]] = m_data[i];
//                
//                // update the time of last read operation
//                m_tLastSample = now;
//        }
//}
//
//lsampl_t AnalogyAnalogInputProxy::value(uint channel)
//{
//        return m_hash[channel];
//}
//
////~~~
//
//AnalogyAnalogInputHardCal::AnalogyAnalogInputHardCal(const char *deviceFile, uint inputSubdevice,
//                                                   uint readChannel, double inputConversionFactor,
//                                                   uint range, uint aref)
//        : AnalogyAnalogIO(deviceFile, inputSubdevice, &readChannel, 1, range, aref),
//          m_inputConversionFactor(inputConversionFactor)
//{
//        initialise();
//}
//
//bool AnalogyAnalogInputHardCal::initialise()
//{
//        // get physical data range for subdevice (min, max, phys. units)
//        m_dataRange = comedi_get_range(m_device, m_subdevice, m_channels[0], m_range);
//        Logger(Debug, "Range for 0x%x (%s):\n\tmin = %g\n\tmax = %g\n", m_dataRange,
//                        (m_dataRange->unit == UNIT_volt ? "volts" : "milliamps"),
//                        m_dataRange->min, m_dataRange->max);
//        if(m_dataRange == NULL) {
//                comedi_perror(m_deviceFile);
//                comedi_close(m_device);
//                return false;
//        }
//        
//        // read max data value
//        m_maxData = comedi_get_maxdata(m_device, m_subdevice, m_channels[0]);
//        Logger(Debug, "Max data = %ld\n", m_maxData);
//}
//
//double AnalogyAnalogInputHardCal::inputConversionFactor() const
//{
//        return m_inputConversionFactor;
//}
//
//double AnalogyAnalogInputHardCal::read()
//{
//        lsampl_t sample;
//        comedi_data_read(m_device, m_subdevice, m_channels[0], m_range, m_aref, &sample);
//        return comedi_to_phys(sample, m_dataRange, m_maxData) * m_inputConversionFactor;
//}
//
////~~~
//
//AnalogyAnalogOutputHardCal::AnalogyAnalogOutputHardCal(const char *deviceFile, uint outputSubdevice,
//                                                     uint writeChannel, double outputConversionFactor, uint aref)
//        : AnalogyAnalogIO(deviceFile, outputSubdevice, &writeChannel, 1, PLUS_MINUS_TEN, aref),
//          m_outputConversionFactor(outputConversionFactor)
//{
//        initialise();
//#ifdef RESET_OUTPUT
//        write(0.0);
//#endif
//}
//
//AnalogyAnalogOutputHardCal::~AnalogyAnalogOutputHardCal()
//{
//#ifdef RESET_OUTPUT
//        write(0.0);
//#endif
//}
//
//bool AnalogyAnalogOutputHardCal::initialise()
//{
//        // get physical data range for subdevice (min, max, phys. units)
//        m_dataRange = comedi_get_range(m_device, m_subdevice, m_channels[0], m_range);
//        Logger(Debug, "Range for 0x%x (%s):\n\tmin = %g\n\tmax = %g\n", m_dataRange,
//                        (m_dataRange->unit == UNIT_volt ? "volts" : "milliamps"),
//                        m_dataRange->min, m_dataRange->max);
//        if(m_dataRange == NULL) {
//                comedi_perror(m_deviceFile);
//                comedi_close(m_device);
//                return false;
//        }
//        
//        // read max data value
//        m_maxData = comedi_get_maxdata(m_device, m_subdevice, m_channels[0]);
//        Logger(Debug, "Max data = %ld\n", m_maxData);
//}
//
//double AnalogyAnalogOutputHardCal::outputConversionFactor() const
//{
//        return m_outputConversionFactor;
//}
//
///**
// * Write the argument to the output of the DAQ board.
// */
//void AnalogyAnalogOutputHardCal::write(double data)
//{
//        lsampl_t sample = comedi_from_phys(data*m_outputConversionFactor, m_dataRange, m_maxData);
//        comedi_data_write(m_device, m_subdevice, m_channels[0], m_range, m_aref, sample);
//}
//
////~~~
//
//AnalogyAnalogIOSoftCal::AnalogyAnalogIOSoftCal(const char *deviceFile, uint subdevice,
//                                             uint *channels, uint nChannels,
//                                             uint range, uint aref)
//        : AnalogyAnalogIO(deviceFile, subdevice, channels, nChannels, range, aref)
//{
//        Logger(Debug, "AnalogyAnalogIOSoftCal::AnalogyAnalogIOSoftCal()\n");
//        if (!readCalibration())
//                throw "Unable to read the calibration of the DAQ board.";
//}
//
//AnalogyAnalogIOSoftCal::~AnalogyAnalogIOSoftCal()
//{
//        Logger(Debug, "AnalogyAnalogIOSoftCal::~AnalogyAnalogIOSoftCal()\n");
//        comedi_cleanup_calibration(m_calibration);
//        delete m_calibrationFile;
//        closeDevice();
//}
//
//bool AnalogyAnalogIOSoftCal::readCalibration()
//{
//        Logger(Debug, "AnalogyAnalogIOSoftCal::readCalibration()\n");
//        m_calibrationFile = comedi_get_default_calibration_path(m_device);
//        if (m_calibrationFile == NULL) {
//                Logger(Critical, "Unable to find a calibration file for [%s].\n", deviceFile());
//                return false;
//        }
//        else {
//                Logger(Debug, "Using calibration file [%s].\n", m_calibrationFile);
//        }
//
//        m_calibration = comedi_parse_calibration_file(m_calibrationFile);
//        if (m_calibration == NULL) {
//                Logger(Critical, "Unable to parse calibration file [%s].\n", m_calibrationFile);
//                return false;
//        }
//        else {
//                Logger(Debug, "Successfully parsed calibration file [%s].\n", m_calibrationFile);
//        }
//        return true;
//}
//
////~~~
//
//AnalogyAnalogInputSoftCal::AnalogyAnalogInputSoftCal(const char *deviceFile, uint inputSubdevice,
//                                                   uint readChannel, double inputConversionFactor,
//                                                   uint range, uint aref)
//        : AnalogyAnalogIOSoftCal(deviceFile, inputSubdevice, &readChannel, 1, range, aref),
//          m_inputConversionFactor(inputConversionFactor)
//{
//        Logger(Debug, "AnalogyAnalogInputSoftCal::AnalogyAnalogInputSoftCal()\n");
//        int flag = comedi_get_softcal_converter(m_subdevice, m_channels[0], m_range,
//                        COMEDI_TO_PHYSICAL, m_calibration, &m_converter);
//        if (flag != 0) {
//                Logger(Critical, "Unable to get converter for sw-calibrated device.\n");
//                throw "Error in comedi_get_softcal_converter()";
//        }
//
//#ifdef ASYNCHRONOUS_INPUT
//        std::stringstream key;
//        key << deviceFile << "-" << inputSubdevice;
//        if (proxies.count(key.str()) == 0) {
//                Logger(Info, "Adding proxy for device %s and subdevice %d.\n", m_deviceFile, inputSubdevice);
//                proxies[key.str()] = new AnalogyAnalogInputProxy(deviceFile, inputSubdevice, &readChannel, 1, range, aref);
//        }
//        else {
//                proxies[key.str()]->addChannel(m_channels[0]);
//        }
//        m_proxy = proxies[key.str()];
//        m_proxy->increaseRefCount();
//#endif
//}
//
//AnalogyAnalogInputSoftCal::~AnalogyAnalogInputSoftCal()
//{
//#ifdef ASYNCHRONOUS_INPUT
//        m_proxy->decreaseRefCount();
//#endif
//}
//
//bool AnalogyAnalogInputSoftCal::initialise()
//{
//#ifndef ASYNCHRONOUS_INPUT
//        return true;
//#else
//        return m_proxy->initialise();
//#endif
//}
//
//double AnalogyAnalogInputSoftCal::inputConversionFactor() const
//{
//        return m_inputConversionFactor;
//}
//
//double AnalogyAnalogInputSoftCal::read()
//{
//#ifndef ASYNCHRONOUS_INPUT
//        lsampl_t sample;
//        comedi_data_read(m_device, m_subdevice, m_channels[0], m_range, m_aref, &sample);
//        return comedi_to_physical(sample, &m_converter) * m_inputConversionFactor;
//#else
//        m_proxy->acquire();
//        return comedi_to_physical(m_proxy->value(m_channels[0]), &m_converter) * m_inputConversionFactor;
//#endif
//}
//
////~~~
//
//AnalogyAnalogOutputSoftCal::AnalogyAnalogOutputSoftCal(const char *deviceFile, uint outputSubdevice,
//                                                     uint writeChannel, double outputConversionFactor,
//                                                     uint aref)
//        : AnalogyAnalogIOSoftCal(deviceFile, outputSubdevice, &writeChannel, 1, PLUS_MINUS_TEN, aref),
//          m_outputConversionFactor(outputConversionFactor)
//{
//        Logger(Debug, "AnalogyAnalogOutputSoftCal::AnalogyAnalogOutputSoftCal()\n");
//        int flag = comedi_get_softcal_converter(m_subdevice, m_channels[0], m_range,
//                        COMEDI_FROM_PHYSICAL, m_calibration, &m_converter);
//        if (flag != 0) {
//                Logger(Critical, "Unable to get converter for sw-calibrated device.\n");
//                throw "Error in comedi_get_softcal_converter()";
//        }
//}
//
//AnalogyAnalogOutputSoftCal::~AnalogyAnalogOutputSoftCal()
//{
//#ifdef RESET_OUTPUT
//        write(0.0);
//#endif
//}
//
//bool AnalogyAnalogOutputSoftCal::initialise()
//{
//#ifdef RESET_OUTPUT
//        write(0.0);
//#endif
//        return true;
//}
//
//double AnalogyAnalogOutputSoftCal::outputConversionFactor() const
//{
//        return m_outputConversionFactor;
//}
//
///**
// * Write the argument to the output of the DAQ board.
// */
//void AnalogyAnalogOutputSoftCal::write(double data)
//{
//        comedi_data_write(m_device, m_subdevice, m_channels[0], m_range, m_aref,
//                comedi_from_physical(data*m_outputConversionFactor, &m_converter));
//}

} // namespace lcg

#endif // HAVE_LIBANALOGY



