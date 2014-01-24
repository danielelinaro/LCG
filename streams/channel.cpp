#include <errno.h>

#include "channel.h"
#include "utils.h"
#include "comedi_io.h"

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

lcg::Stream* InputChannelFactory(string_dict& args)
{
        uint subdevice, channel, range, reference, id;
        std::string device, rangeStr, referenceStr, units;
        double conversionFactor, samplingRate;

        id = lcg::GetIdFromDictionary(args);

        if ( ! lcg::CheckAndExtractValue(args, "device", device) ||
             ! lcg::CheckAndExtractUnsignedInteger(args, "subdevice", &subdevice) ||
             ! lcg::CheckAndExtractUnsignedInteger(args, "channel", &channel) ||
             ! lcg::CheckAndExtractDouble(args, "conversionFactor", &conversionFactor)) {
                lcg::Logger(lcg::Critical, "Unable to build an input channel.\n");
                return NULL;
        }

        if (! lcg::CheckAndExtractDouble(args, "samplingRate", &samplingRate))
                samplingRate = 1/lcg::GetGlobalDt();

        if (! lcg::CheckAndExtractValue(args, "range", rangeStr)) {
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
                        lcg::Logger(lcg::Critical, "Unknown input range: [%s].\n", rangeStr.c_str());
                        lcg::Logger(lcg::Critical, "Unable to build an analog input.\n");
                        return NULL;
                }
        }

        if (! lcg::CheckAndExtractValue(args, "reference", referenceStr)) {
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
                        lcg::Logger(lcg::Critical, "Unknown reference mode: [%s].\n", referenceStr.c_str());
                        lcg::Logger(lcg::Critical, "Unable to build an input channel.\n");
                        return NULL;
                }
        }

        if (! lcg::CheckAndExtractValue(args, "units", units)) {
                units = "mV";
        }

        return new lcg::InputChannel(device.c_str(), subdevice, range, reference,
                                     channel, conversionFactor, samplingRate, units.c_str(), id);
}

lcg::Stream* OutputChannelFactory(string_dict& args)
{
        uint subdevice, channel, reference, id;
        std::string device, referenceStr, units, stimfile;
        double conversionFactor, samplingRate, offset;
        bool resetOutput;

        id = lcg::GetIdFromDictionary(args);

        if ( ! lcg::CheckAndExtractValue(args, "device", device) ||
             ! lcg::CheckAndExtractUnsignedInteger(args, "subdevice", &subdevice) ||
             ! lcg::CheckAndExtractUnsignedInteger(args, "channel", &channel) ||
             ! lcg::CheckAndExtractValue(args, "stimfile", stimfile) ||
             ! lcg::CheckAndExtractDouble(args, "conversionFactor", &conversionFactor)) {
                lcg::Logger(lcg::Critical, "Unable to build an output channel.\n");
                return NULL;
        }

        if (! lcg::CheckAndExtractDouble(args, "samplingRate", &samplingRate))
                samplingRate = 1/lcg::GetGlobalDt();

        if (! lcg::CheckAndExtractValue(args, "reference", referenceStr)) {
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
                        lcg::Logger(lcg::Critical, "Unknown reference mode: [%s].\n", referenceStr.c_str());
                        lcg::Logger(lcg::Critical, "Unable to build an analog output.\n");
                        return NULL;
                }
        }

        if (! lcg::CheckAndExtractValue(args, "units", units)) {
                units = "pA";
        }

        if (! lcg::CheckAndExtractDouble(args, "offset", &offset)) {
                offset = 0.;
        }

        if (! lcg::CheckAndExtractBool(args, "resetOutput", &resetOutput)) {
                resetOutput = true;
        }

        return new lcg::OutputChannel(device.c_str(), subdevice, PLUS_MINUS_TEN, reference,
                                      channel, conversionFactor, samplingRate, units.c_str(),
                                      stimfile.c_str(), offset, resetOutput, id);
}

namespace lcg {

std::map<std::string,ComediDevice*> devices;

///// HELPER FUNCTIONS AND STRUCTURES - START /////

struct input_loop_data {
        input_loop_data(comedi_t *dev, int subdev, char *buf, size_t buflen, comedi_polynomial_t *conv, std::vector<InputChannel*>* chan)
                : device(dev), subdevice(subdev), buffer(buf), buffer_length(buflen), converters(conv), channels(chan) {}
        comedi_t *device;
        uint subdevice;
        char *buffer;
        size_t buffer_length;
        comedi_polynomial_t *converters;
        std::vector<InputChannel*>* channels;
};

struct output_loop_data {
        output_loop_data(comedi_t *dev, int subdev, const char *buf, size_t buflen, size_t bytes_already_written, int nchan, int bps)
                : device(dev), subdevice(subdev), buffer(buf), buffer_length(buflen), bytes_written(bytes_already_written),
                  n_channels(nchan), bytes_per_sample(bps) {}
        comedi_t *device;
        uint subdevice;
        const char *buffer;
        size_t buffer_length, bytes_written;
        int n_channels, bytes_per_sample;
};

void* input_loop(void *arg)
{
        int i, j, k, ret, cnt, total, n_channels, bytes_per_sample, flags;
        int *nsteps = new int;
        lsampl_t sample;
        input_loop_data *data = static_cast<input_loop_data*>(arg);
        cnt = total = *nsteps = 0;
        if (!data)
                pthread_exit((void *) nsteps);
        n_channels = data->channels->size();
        flags = comedi_get_subdevice_flags(data->device, data->subdevice);
        if (flags & SDF_LSAMPL)
                bytes_per_sample = sizeof(lsampl_t);
        else
                bytes_per_sample = sizeof(sampl_t);
        while (!KILL_PROGRAM() && (ret = read(comedi_fileno(data->device), data->buffer, data->buffer_length)) > 0) {
                total += ret;
                Logger(Debug, "Read %d bytes from the board [%d/%d].\r", ret, total, data->buffer_length);
                for (i=0; i<ret/bytes_per_sample; i++) {
                        if (flags & SDF_LSAMPL)
                                sample = ((lsampl_t *) data->buffer)[i];
                        else
                                sample = ((sampl_t *) data->buffer)[i];
                        j = cnt%n_channels;
                        k = cnt/n_channels;
                        cnt++;
                        data->channels->at(j)->at(k) = comedi_to_physical(sample, &data->converters[j]) *
                                data->channels->at(j)->conversionFactor();
                }
        }
        Logger(Debug, "\n");
        *nsteps = k+1;
        pthread_exit((void *) nsteps);
}

void* output_loop(void *arg)
{
        size_t bytes_to_write, bytes_written;
        int *nsteps = new int;
        output_loop_data *data = static_cast<output_loop_data*>(arg);
        *nsteps = 0;
        if (!data)
                pthread_exit((void *) nsteps); // this value we return is wrong, but if we get here,
                                               // there is probably some other problem.
        *nsteps = data->bytes_written / (data->n_channels*data->bytes_per_sample);
        bytes_to_write = data->buffer_length - data->bytes_written;
        while (!KILL_PROGRAM() && bytes_to_write > 0) {
                bytes_written = write(comedi_fileno(data->device), (void *) (data->buffer+data->bytes_written), bytes_to_write);
                if (bytes_written < 0) {
                        Logger(Critical, "Error while writing: %s\n", strerror(errno));
                        break;
                }
                bytes_to_write -= bytes_written;
                data->bytes_written += bytes_written;
        }
        *nsteps = data->bytes_written / (data->n_channels*data->bytes_per_sample);
        pthread_exit((void *) nsteps);
}

char *cmd_src(int src,char *buf)
{
	buf[0]=0;
	if (src & TRIG_NONE) strcat(buf,"none|");
	if (src & TRIG_NOW) strcat(buf,"now|");
	if (src & TRIG_FOLLOW) strcat(buf, "follow|");
	if (src & TRIG_TIME) strcat(buf, "time|");
	if (src & TRIG_TIMER) strcat(buf, "timer|");
	if (src & TRIG_COUNT) strcat(buf, "count|");
	if (src & TRIG_EXT) strcat(buf, "ext|");
	if (src & TRIG_INT) strcat(buf, "int|");
#ifdef TRIG_OTHER
	if (src & TRIG_OTHER) strcat(buf, "other|");
#endif
	if (strlen(buf) == 0)
	        sprintf(buf, "unknown(0x%08x)", src);
	else
		buf[strlen(buf)-1] = 0;
	return buf;
}

void dump_cmd(LogLevel level, comedi_cmd *cmd)
{
	char buf[100];
	Logger(level, "subdev:      %d\n", cmd->subdev);
	Logger(level, "flags:       %x\n", cmd->flags);
	Logger(level, "start:      %-8s %d\n", cmd_src(cmd->start_src,buf), cmd->start_arg);
	Logger(level, "scan_begin: %-8s %d\n", cmd_src(cmd->scan_begin_src,buf), cmd->scan_begin_arg);
	Logger(level, "convert:    %-8s %d\n", cmd_src(cmd->convert_src,buf), cmd->convert_arg);
	Logger(level, "scan_end:   %-8s %d\n", cmd_src(cmd->scan_end_src,buf), cmd->scan_end_arg);
	Logger(level, "stop:       %-8s %d\n", cmd_src(cmd->stop_src,buf), cmd->stop_arg);
	Logger(level, "chanlist_len: %d\n", cmd->chanlist_len);
	Logger(level, "data_len: %d\n", cmd->data_len);
}

///// HELPER FUNCTIONS - END /////

///// CHANNEL CLASSES - START /////

Channel::Channel(const char *device, uint channel, double samplingRate, const char *units, uint id)
        : Stream(id), m_channel(channel), m_samplingRate(samplingRate)
{
        strncpy(m_device, device, FILENAME_MAXLEN);
        setName("Channel");
        setUnits(units);
        m_parameters["channel"] = (double) m_channel;
        m_parameters["sampling_rate"] = m_samplingRate;
}

Channel::~Channel()
{}

const char* Channel::device() const
{
        return m_device;
}

uint Channel::channel() const
{
        return m_channel;
}

double Channel::samplingRate() const
{
        return m_samplingRate;
}

ComediChannel::ComediChannel(const char *device, uint subdevice, uint range, uint reference,
                uint channel, double conversionFactor, double samplingRate, const char *units, uint id)
        : Channel(device, channel, samplingRate, units, id),
          m_subdevice(subdevice), m_range(range), m_reference(reference),
          m_conversionFactor(conversionFactor), m_validDataLength(0)
{
        setName("ComediChannel");
        m_parameters["subdevice"] = (double) m_subdevice;
        m_parameters["range"] = (double) m_range;
        m_parameters["reference"] = (double) m_reference;
        m_parameters["conversion_factor"] = m_conversionFactor;
}

uint ComediChannel::subdevice() const
{
        return m_subdevice;
}

uint ComediChannel::range() const
{
        return m_range;
}

uint ComediChannel::reference() const
{
        return m_reference;
}

double ComediChannel::conversionFactor() const
{
        return m_conversionFactor;
}

bool ComediChannel::initialise()
{
        std::string dev = device();
        if (!devices.count(dev))
                devices[dev] = new ComediDevice(device());
        devices[dev]->addChannel(this);
        return true;
}

void ComediChannel::terminate()
{
        bool erase = false;
        if (devices[device()]->numberOfChannels() == 1)
                erase = true;
        devices[device()]->removeChannel(this);
        if (erase)
                devices.erase(device());
}

void ComediChannel::run(double tend)
{
        devices[device()]->acquire(tend);
}

void ComediChannel::join(int *err)
{
        devices[device()]->join(err);
}

InputChannel::InputChannel(const char *device, uint subdevice, uint range, uint reference,
        uint channel, double conversionFactor, double samplingRate, const char *units, uint id)
        : ComediChannel(device, subdevice, range, reference, channel, conversionFactor, samplingRate, units, id),
          m_data(NULL), m_dataLength(0)
{
        setName("InputChannel");
}

InputChannel::~InputChannel()
{
        if (m_data) {
                Logger(Debug, "Deallocating data from InputChannel.\n");
                delete m_data;
        }
}

const double* InputChannel::data(size_t *length) const
{
        *length = m_validDataLength;
        return m_data;
}

double& InputChannel::operator[](int i)
{
        return this->at(i);
}

const double& InputChannel::operator[](int i) const
{
        return this->at(i);
}

double& InputChannel::at(int i)
{
        if (i<0 || i>=m_dataLength)
                throw "Index out of bounds";
        return m_data[i];
}

const double& InputChannel::at(int i) const
{
        if (i<0 || i>=m_dataLength)
                throw "Index out of bounds";
        return m_data[i];
}

void InputChannel::run(double tend)
{
        allocateDataBuffer(tend);
        ComediChannel::run(tend);
}

bool InputChannel::allocateDataBuffer(double tend)
{
        m_dataLength = m_validDataLength = ceil(tend*samplingRate());
        try {
                m_data = new double[m_dataLength];
                if (m_data)
                        return true;
        } catch(...) {}
        // we get here only if there were problems in allocating memory
        m_dataLength = m_validDataLength = 0;
        m_data = NULL;
        return false;
}

OutputChannel::OutputChannel(const char *device, uint subdevice, uint range, uint reference,
                uint channel, double conversionFactor, double samplingRate, const char *units,
                const char *stimfile, double offset, bool resetOutput, uint id)
        : ComediChannel(device, subdevice, range, reference, channel, conversionFactor, samplingRate, units, id),
          m_stimulus(1./samplingRate, stimfile), m_offset(offset), m_resetOutput(resetOutput)
{
        m_validDataLength = m_stimulus.length();
        setName("OutputChannel");
}

void OutputChannel::terminate()
{
        double lastOutput = m_stimulus[m_stimulus.length()-1];
        if (m_resetOutput) {
                Logger(Debug, "OutputChannel::terminate >> resetting the output.\n");
                comedi_t *dev;
                char *calibrationFile;
                comedi_calibration_t *calibration;
                comedi_polynomial_t converter;
                dev = comedi_open(device());
                if (dev == NULL) {
                        Logger(Important, "Unable to reset the output: error in comedi_open: %s.\n", comedi_strerror(comedi_errno()));
                        goto write_log_file;
                }
                calibrationFile = comedi_get_default_calibration_path(dev);
                if (calibrationFile == NULL) {
                        Logger(Important, "Unable to reset the output: error in comedi_get_default_calibration_path: %s.\n", comedi_strerror(comedi_errno()));
                        comedi_close(dev);
                        goto write_log_file;
                }
                calibration = comedi_parse_calibration_file(calibrationFile);
                if (calibration == NULL) {
                        Logger(Important, "Unable to reset the output: error in comedi_parse_calibration_file: %s.\n", comedi_strerror(comedi_errno()));
                        free(calibrationFile);
                        comedi_close(dev);
                        goto write_log_file;
                }
                if (comedi_get_softcal_converter(subdevice(), channel(), range(), COMEDI_FROM_PHYSICAL, calibration, &converter) != 0) {
                        Logger(Important, "Unable to reset the output: error in comedi_get_softcal_converter: %s.\n", comedi_strerror(comedi_errno()));
                        comedi_cleanup_calibration(calibration);
                        free(calibrationFile);
                        comedi_close(dev);
                        goto write_log_file;
                }
                if (comedi_data_write(dev, subdevice(), channel(), range(), reference(), comedi_from_physical(0., &converter)) == 1)
                        lastOutput = 0.0;
                else
                        Logger(Important, "Unable to reset the output: error in comedi_data_write: %s.\n", comedi_strerror(comedi_errno()));
                comedi_cleanup_calibration(calibration);
                free(calibrationFile);
                comedi_close(dev);
        }
write_log_file:
        FILE *fid = fopen(LOGFILE,"w");
        if (fid != NULL) {
                fprintf(fid, "%f", lastOutput);
                fclose(fid);
        }
}

const Stimulus* OutputChannel::stimulus() const
{
        return &m_stimulus;
}

const char* OutputChannel::stimulusFile() const
{
        return m_stimulus.stimulusFile();
}

bool OutputChannel::setStimulusFile(const char *filename)
{
        return m_stimulus.setStimulusFile(filename);
}

double& OutputChannel::operator[](int i)
{
        return m_stimulus[i];
}

const double& OutputChannel::operator[](int i) const
{
        return m_stimulus[i];
}

double& OutputChannel::at(int i)
{
        return m_stimulus[i];
}

const double& OutputChannel::at(int i) const
{
        return m_stimulus[i];
}

bool OutputChannel::hasMetadata(size_t *ndims) const
{
        *ndims = 2;
        return true;
}

const double* OutputChannel::metadata(size_t *dims, char *label) const
{
        sprintf(label, "Stimulus_Matrix");
        return m_stimulus.metadata(&dims[0], &dims[1]);
}

const double* OutputChannel::data(size_t *length) const
{
        size_t unused;
        *length = m_validDataLength;
        return m_stimulus.data(&unused);
}

double OutputChannel::offset() const
{
        return m_offset;
}

ComediDevice::ComediDevice(const char *device, bool autoDestroy)
        : m_subdevices(), m_autoDestroy(autoDestroy),
          m_acquiring(false), m_joined(false), m_err(0),
          m_numberOfChannels(0)
{
        strcpy(m_device, device);
        Logger(Debug, "Created ComediDevice [%s].\n", m_device);
}

ComediDevice::~ComediDevice()
{
        Logger(Debug, "Destroyed ComediDevice [%s].\n", m_device);
}

bool ComediDevice::isSameDevice(ComediChannel *channel) const
{
        return strcmp(channel->device(),m_device) == 0;
}

bool ComediDevice::isChannelPresent(ComediChannel *channel) const
{
        if (isSameDevice(channel) &&
            m_subdevices.count(channel->subdevice()) &&
            m_subdevices.find(channel->subdevice())->second.count(channel->channel()))
                return true;
        return false;
}

bool ComediDevice::addChannel(ComediChannel *channel)
{
        if (isChannelPresent(channel))
                return false;
        m_subdevices[channel->subdevice()][channel->channel()] = channel;
        m_numberOfChannels++;
        Logger(Debug, "Added channel %d on subdevice %d.\n", channel->channel(), channel->subdevice());
        return true;
}

bool ComediDevice::removeChannel(ComediChannel *channel)
{
        if (isChannelPresent(channel)) {
                m_subdevices[channel->subdevice()].erase(channel->channel());
                m_numberOfChannels--;
                Logger(Debug, "Removed channel %d on subdevice %d.\n", channel->channel(), channel->subdevice());
                if (m_subdevices[channel->subdevice()].empty()) {
                        m_subdevices.erase(channel->subdevice());
                        Logger(Debug, "Removed subdevice %d.\n", channel->subdevice());
                }
                if (m_autoDestroy && m_subdevices.empty()) {
                        Logger(Debug, "Removed device %s.\n", channel->device());
                        delete this;
                }
                return true;
        }
        return false;
}

void ComediDevice::acquire(double tend)
{
        if (m_acquiring) {
                Logger(Debug, "Already acquiring data from device %s.\n", m_device);
                return;
        }
        m_err = 0;
        m_tend = tend;
        Logger(Debug, "Will start acquiring data from device %s.\n", m_device);
        int err = pthread_create(&m_ioThread, NULL, IOThread, (void *) this);
        if (err) {
                Logger(Critical, "Unable to start the I/O thread.\n");
                m_err = -1;
                return;
        }
        Logger(Info, "Expected duration: %.2f seconds.\n", tend);
        m_acquiring = true;
        m_joined = false;
}

void ComediDevice::join(int *err)
{
        if (!m_joined) {
                int *err;
                pthread_join(m_ioThread, (void **) &err);
                m_err = *err;
                m_joined = true;
                delete err;
        }
        *err = m_err;
}

size_t ComediDevice::numberOfChannels() const
{
        return m_numberOfChannels;
}

void* ComediDevice::IOThread(void *arg)
{
        ComediDevice *self = static_cast<ComediDevice*>(arg);
        comedi_t *device;
        char *calibration_file;
        comedi_calibration_t *calibration;
        
        comedi_polynomial_t *in_converters, *out_converters;
        uint *in_chanlist, *out_chanlist, in_subdevice, out_subdevice, in_insn_data, out_insn_data;
        int i, j, k, nsteps, ret;
        int in_bytes_per_sample, out_bytes_per_sample;
        comedi_cmd cmd;
        comedi_insn in_insn, out_insn;
        size_t input_buffer_length, output_buffer_length;
        size_t bytes_written;
        char *input_buffer, *output_buffer;
        pthread_t in_loop_thrd, out_loop_thrd;
        int *in_nsteps = NULL, *out_nsteps = NULL, *err = new int;
        input_loop_data *in_data;
        output_loop_data *out_data;
        size_t n_input_channels, n_output_channels;
        std::vector<InputChannel*> input_channels;
        std::vector<OutputChannel*> output_channels;

        // indicates that there's been an error
        *err = -1;

        if (!self)
                pthread_exit((void *) err);

        if (self->m_subdevices.empty()) {
                Logger(Critical, "No input or output channels specified.\n");
                pthread_exit((void *) err);
        }

        // open the device
        device = comedi_open(self->m_device);
        if (device == NULL) {
                Logger(Critical, "Unable to open device [%s].\n", self->m_device);
                pthread_exit((void *) err);
        }
        Logger(Debug, "Successfully opened device [%s].\n", self->m_device);

        // get path of calibration file
        calibration_file = comedi_get_default_calibration_path(device);
        if (calibration_file == NULL) {
                Logger(Critical, "Unable to find default calibration file.\n");
                comedi_close(device);
                pthread_exit((void *) err);
        }
        Logger(Debug, "The default calibration file is [%s].\n", calibration_file);

        // parse the calibration file
        calibration = comedi_parse_calibration_file(calibration_file);
        if (calibration == NULL) {
                Logger(Critical, "Unable to parse calibration file.\n");
                comedi_close(device);
                free(calibration_file);
                pthread_exit((void *) err);
        }
        Logger(Debug, "Successfully parsed calibration file.\n");

        // split the channels into input and output channels
        std::map< int, std::map<int, ComediChannel*> >::iterator subdev_it;
        for (subdev_it=self->m_subdevices.begin(); subdev_it!=self->m_subdevices.end(); subdev_it++) {
                std::map<int, ComediChannel*>::iterator channel_it;
                for (channel_it=subdev_it->second.begin(); channel_it!=subdev_it->second.end(); channel_it++) {
                        if (dynamic_cast<InputChannel*>(channel_it->second))
                                input_channels.push_back(dynamic_cast<InputChannel*>(channel_it->second));
                        else
                                output_channels.push_back(dynamic_cast<OutputChannel*>(channel_it->second));
                }
        }
        n_input_channels = input_channels.size();
        n_output_channels = output_channels.size();
        Logger(Debug, "There are %d input channel(s) and %d output channel(s).\n", n_input_channels, n_output_channels);

        if (n_input_channels) {

                in_subdevice = input_channels[0]->subdevice();
                in_chanlist = new uint[n_input_channels];
                in_converters = new comedi_polynomial_t[n_input_channels];
                // pack channel numbers and get a converter for each channel
                for (i=0; i<n_input_channels; i++) {
                        in_chanlist[i] = CR_PACK(input_channels[i]->channel(),
                                                 input_channels[i]->range(),
                                                 input_channels[i]->reference());
                        ret = comedi_get_softcal_converter(in_subdevice,
                                                           input_channels[i]->channel(),
                                                           input_channels[i]->range(),
                                                           COMEDI_TO_PHYSICAL, calibration, &in_converters[i]);
                        if (ret < 0) {
                                Logger(Critical, "Unable to get converter for channel %d.\n", input_channels[i]->channel());
                                goto end_io;
                        }
                        Logger(Debug, "Successfully obtained converter for channel %d.\n", input_channels[i]->channel());
                }
                                
                if (comedi_get_subdevice_flags(device, in_subdevice) & SDF_LSAMPL)
                        in_bytes_per_sample = sizeof(lsampl_t);
                else
                        in_bytes_per_sample = sizeof(sampl_t);
                Logger(Debug, "Number of bytes per input sample: %d\n", in_bytes_per_sample);
                
                memset(&cmd, 0, sizeof(struct comedi_cmd_struct));
                ret = comedi_get_cmd_generic_timed(device, in_subdevice, &cmd,
                                n_input_channels, NSEC_PER_SEC/input_channels[0]->samplingRate());
                if (ret < 0) {
                        Logger(Critical, "Error in comedi_get_cmd_generic_timed.\n");
                        goto end_io;
                }
                cmd.convert_arg = 0;    // this value is wrong, but will be fixed by comedi_command_test
                cmd.chanlist    = in_chanlist;
                cmd.start_src   = TRIG_INT;
                cmd.stop_src    = TRIG_COUNT;
                cmd.stop_arg    = ceil(self->m_tend*input_channels[0]->samplingRate());
                
                // test and fix the command
                if (comedi_command_test(device, &cmd) < 0 &&
                    comedi_command_test(device, &cmd) < 0) {
                        Logger(Critical, "Unable to setup the command.\n");
                        goto end_io;
                }
                Logger(Debug, "Successfully setup the command.\n");
                Logger(Debug, "-------------------------------\n");
                dump_cmd(Debug, &cmd);
                Logger(Debug, "-------------------------------\n");
        
                // issue the command (acquisition doesn't start immediately)
                ret = comedi_command(device, &cmd);
                if (ret < 0) {
                        Logger(Critical, "Unable to issue the command.\n");
                        comedi_perror("comedi_command");
                        goto end_io;
                }
                Logger(Debug, "Successfully issued the command.\n");
        
                // allocate memory for reading at most one second of data
                input_buffer_length = ceil(MIN(self->m_tend,1.)*input_channels[0]->samplingRate()) *
                        n_input_channels * in_bytes_per_sample;
                input_buffer = new char[input_buffer_length];
                Logger(Debug, "The total size of the input buffer is %ld bytes (= %.2f Mb).\n",
                                input_buffer_length, (double) input_buffer_length/(1024*1024));

                // prepare the triggering instruction
                in_insn_data = 0;
                memset(&in_insn, 0, sizeof(in_insn));
                in_insn.insn = INSN_INTTRIG;
                in_insn.n = 1;
                in_insn.data = &in_insn_data;
                in_insn.subdev = in_subdevice;
        }

        if (n_output_channels) {

                out_subdevice = output_channels[0]->subdevice();
                out_chanlist = new uint[n_output_channels];
                out_converters = new comedi_polynomial_t[n_output_channels];
                // pack channel numbers and get a converter for each channel
                for (i=0; i<n_output_channels; i++) {
                        out_chanlist[i] = CR_PACK(output_channels[i]->channel(),
                                                  output_channels[i]->range(),
                                                  output_channels[i]->reference());
                        ret = comedi_get_softcal_converter(out_subdevice,
                                                           output_channels[i]->channel(),
                                                           output_channels[i]->range(),
                                                           COMEDI_FROM_PHYSICAL, calibration, &out_converters[i]);
                        if (ret < 0) {
                                Logger(Critical, "Unable to get converter for channel %d.\n", output_channels[i]->channel());
                                goto end_io;
                        }
                        Logger(Debug, "Successfully obtained converter for channel %d.\n", output_channels[i]->channel());
                }
                                
                int flags = comedi_get_subdevice_flags(device, out_subdevice);
                if (flags & SDF_LSAMPL)
                        out_bytes_per_sample = sizeof(lsampl_t);
                else
                        out_bytes_per_sample = sizeof(sampl_t);
                Logger(Debug, "Number of bytes per output sample: %d\n", out_bytes_per_sample);
                
                memset(&cmd, 0, sizeof(struct comedi_cmd_struct));
                ret = comedi_get_cmd_generic_timed(device, out_subdevice, &cmd,
                                n_output_channels, NSEC_PER_SEC/output_channels[0]->samplingRate());
                if (ret < 0) {
                        Logger(Critical, "Error in comedi_get_cmd_generic_timed.\n");
                        goto end_io;
                }
                cmd.convert_arg = 0;    // this value is wrong, but will be fixed by comedi_command_test
                cmd.chanlist    = out_chanlist;
                cmd.start_src   = TRIG_INT;
                cmd.stop_src    = TRIG_COUNT;
                cmd.stop_arg    = ceil(self->m_tend*output_channels[0]->samplingRate());
                
                // test and fix the command
                if (comedi_command_test(device, &cmd) < 0 &&
                    comedi_command_test(device, &cmd) < 0) {
                        Logger(Critical, "Unable to setup the command.\n");
                        goto end_io;
                }
                Logger(Debug, "Successfully setup the command.\n");
                Logger(Debug, "-------------------------------\n");
                dump_cmd(Debug, &cmd);
                Logger(Debug, "-------------------------------\n");
        
                // issue the command (writing doesn't start immediately)
                ret = comedi_command(device, &cmd);
                if (ret < 0) {
                        Logger(Critical, "Unable to issue the command.\n");
                        comedi_perror("comedi_command");
                        goto end_io;
                }
                Logger(Debug, "Successfully issued the command.\n");
        
                // allocate memory for the data that will be written
                output_buffer_length = ceil(self->m_tend*output_channels[0]->samplingRate()) *
                        n_output_channels * out_bytes_per_sample;
                output_buffer = new char[output_buffer_length];
                Logger(Debug, "The total size of the output buffer is %ld bytes (= %.2f Mb).\n",
                                output_buffer_length, (double) output_buffer_length/(1024*1024));
		
                // fill the buffer and FULLY preload it
                nsteps = ceil(self->m_tend*output_channels[0]->samplingRate());
                double sample;
                sampl_t *ptr = (sampl_t*) output_buffer;
                lsampl_t *lptr = (lsampl_t*) output_buffer;
                for (i=0; i<nsteps; i++) {
                        for (j=0; j<n_output_channels; j++) {
                                sample = (output_channels[j]->offset() + output_channels[j]->at(i))
                                        * output_channels[j]->conversionFactor();
                                if (flags & SDF_LSAMPL)
                                        *lptr = comedi_from_physical(sample, &out_converters[j]);
                                else
                                        *ptr = (sampl_t) comedi_from_physical(sample, &out_converters[j]);
                                ptr++;
                                lptr++;
                        }
                }

                bytes_written = write(comedi_fileno(device), (void *) output_buffer, output_buffer_length);
                if (bytes_written < 0)
                        Logger(Critical, "Error on write: %s.\n", strerror(errno));
                Logger(Debug, "Number of preloaded bytes: %d.\n", bytes_written);

                out_insn_data = 0;
                memset(&out_insn, 0, sizeof(out_insn));
                out_insn.insn = INSN_INTTRIG;
                out_insn.n = 1;
                out_insn.data = &out_insn_data;
                out_insn.subdev = out_subdevice;
        }

        *err = 0;

        if (n_input_channels) {
                // start the acquisition
                ret = comedi_do_insn(device, &in_insn);
                if (ret < 0)
                        Logger(Critical, "Unable to start the acquisition.\n");
                else
                        Logger(Debug, "Successfully started the acquisition.\n");
                in_data = new input_loop_data(device, in_subdevice, input_buffer, input_buffer_length, in_converters, &input_channels);
                pthread_create(&in_loop_thrd, NULL, input_loop, (void *) in_data);
        }

        if (n_output_channels) {
                // start the writing
                ret = comedi_do_insn(device, &out_insn);
                if (ret < 0)
                        Logger(Critical, "Unable to start the writing: %s.\n", comedi_strerror(comedi_errno()));
                else
                        Logger(Debug, "Successfully started the writing.\n");
                if (bytes_written < output_buffer_length) {
                        // this part doesn't actually need a separate thread...
                        Logger(Debug, "There are additional samples to be written.\n");
                        out_data = new output_loop_data(device, out_subdevice, output_buffer, output_buffer_length,
                                        bytes_written, n_output_channels, out_bytes_per_sample);
                        pthread_create(&out_loop_thrd, NULL, output_loop, (void *) out_data);
                        Logger(Debug, "Waiting for output thread to complete.\n");
                        pthread_join(out_loop_thrd, (void **) &out_nsteps);
                        *err = !(*out_nsteps);
                        for (int i=0; i<output_channels.size(); i++)
                                output_channels[i]->m_validDataLength = *out_nsteps;
                        delete out_nsteps;
                        delete out_data;
                }
        }

        if (n_input_channels) {
                Logger(Debug, "Waiting for input thread to complete...\n");
                pthread_join(in_loop_thrd, (void **) &in_nsteps);
                if (*err == 0 && *in_nsteps > 0) // no previous errors and we have read something
                        *err = 0;
                else
                        *err = -1;
                for (int i=0; i<input_channels.size(); i++)
                        input_channels[i]->m_validDataLength = *in_nsteps;
                delete in_nsteps;
                delete in_data;
        }

        if (n_input_channels) comedi_cancel(device, in_subdevice);
        if (n_output_channels) comedi_cancel(device, out_subdevice);

end_io:
        if (n_input_channels) {
                delete in_chanlist;
                delete in_converters;
                delete input_buffer;
        }
        if (n_output_channels) {
                delete out_chanlist;
                delete out_converters;
                delete output_buffer;
        }

        comedi_cleanup_calibration(calibration);
        Logger(Debug, "Cleaned up the calibration.\n");
        free(calibration_file);
        comedi_close(device);
        Logger(Debug, "Closed device [%s].\n", self->m_device);
        self->m_acquiring = false;

        pthread_exit((void *) err);
}

}

///// CHANNEL CLASSES - END /////

