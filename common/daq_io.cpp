#include "daq_io.h"
#include "utils.h"
#include <errno.h>

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

using namespace lcg;

///// CHANNEL CLASSES - START /////

Channel::Channel(const char *device, uint subdevice, uint range, uint reference,
                uint channel, double conversionFactor, double samplingRate, const char *units)
        : m_subdevice(subdevice), m_range(range), m_reference(reference),
          m_conversionFactor(conversionFactor), m_samplingRate(samplingRate), m_channel(channel) {
        strncpy(m_device, device, FILENAME_MAXLEN);
        strncpy(m_units, units, 10);
}
Channel::~Channel() {}
const char* Channel::device() const { return m_device; }
const char* Channel::units() const { return m_units; }
uint Channel::subdevice() const { return m_subdevice; }
uint Channel::range() const { return m_range; }
uint Channel::reference() const { return m_reference; }
uint Channel::channel() const { return m_channel; }
double Channel::conversionFactor() const { return m_conversionFactor; }
double Channel::samplingRate() const { return m_samplingRate; }

InputChannel::InputChannel(const char *device, uint subdevice, uint range, uint reference,
        uint channel, double conversionFactor, double samplingRate, const char *units)
        : Channel(device, subdevice, range, reference, channel, conversionFactor, samplingRate, units),
          m_data(NULL), m_dataLength(0)
{}

InputChannel::~InputChannel()
{
        if (m_data) {
                Logger(Debug, "Deallocating data from InputChannel.\n");
                delete m_data;
        }
}

const double* InputChannel::data(size_t *length) const
{
        *length = m_dataLength;
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

bool InputChannel::allocateDataBuffer(double tend)
{
        m_dataLength = ceil(tend*samplingRate());
        try {
                m_data = new double[m_dataLength];
                if (m_data)
                        return true;
        } catch(...) {}
        // we get here only if there were problems in allocating memory
        m_dataLength = 0;
        m_data = NULL;
        return false;
}

OutputChannel::OutputChannel(const char *device, uint subdevice, uint range, uint reference,
                uint channel, double conversionFactor, double samplingRate, const char *units, const char *stimfile)
        : Channel(device, subdevice, range, reference, channel, conversionFactor, samplingRate, units),
          m_stimulus(1./samplingRate, stimfile)
{}

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

///// CHANNEL CLASSES - END /////

static char *cmd_src(int src,char *buf)
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

static void dump_cmd(LogLevel level, comedi_cmd *cmd)
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

struct io_loop_data {
        io_loop_data(comedi_t *dev, int subdev, char *buf, size_t buflen, comedi_polynomial_t *conv, std::vector<Channel*>* chan)
                : device(dev), subdevice(subdev), buffer(buf), buffer_length(buflen), converters(conv), channels(chan) {}
        comedi_t *device;
        uint subdevice;
        char *buffer;
        size_t buffer_length;
        comedi_polynomial_t *converters;
        std::vector<Channel*>* channels;
};

void* input_loop(void *arg)
{
        int i, j, k, ret, cnt, total, n_channels, bytes_per_sample, flags;
        int *nsteps = new int;
        lsampl_t sample;
        io_loop_data *data = static_cast<io_loop_data*>(arg);
        cnt = total = *nsteps = 0;
        if (!data)
                pthread_exit((void *) nsteps);
        n_channels = data->channels->size();
        flags = comedi_get_subdevice_flags(data->device, data->subdevice);
        if (flags & SDF_LSAMPL)
                bytes_per_sample = sizeof(lsampl_t);
        else
                bytes_per_sample = sizeof(sampl_t);
        while ((ret = read(comedi_fileno(data->device), data->buffer, data->buffer_length)) > 0) {
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
        int *nsteps = new int;
        *nsteps = 10000000;
        pthread_exit((void *) nsteps);
}

void* io_thread(void *in)
{
        comedi_t *device;
        char *calibration_file;
        const char *io_device;
        comedi_calibration_t *calibration;
        comedi_polynomial_t *in_converters, *out_converters;
        uint *in_chanlist, *out_chanlist, in_subdevice, out0.001_subdevice, in_insn_data, out_insn_data;
        int i, j, k, nsteps, ret;
        int in_bytes_per_sample, out_bytes_per_sample;
        comedi_cmd cmd;
        comedi_insn in_insn, out_insn;
        size_t n_input_channels, n_output_channels;
        size_t input_buffer_length, output_buffer_length;
        size_t bytes_written;
        char *input_buffer, *output_buffer;
        pthread_t in_loop_thrd, out_loop_thrd;
        int *in_retval, *out_retval, *retval = new int;
        io_loop_data *in_data, *out_data;
        std::vector<Channel*> input_channels, output_channels;

        io_thread_arg *arg = static_cast<io_thread_arg*>(in);
        *retval = -1;

        if (!arg)
                pthread_exit((void *) retval);

        n_input_channels = arg->input_channels->size();
        n_output_channels = arg->output_channels->size();
        if (n_input_channels == 0 && n_output_channels == 0) {
                Logger(Critical, "No input or output channels specified.\n");
                pthread_exit((void *) retval);
        }
        if (n_input_channels)
                io_device = arg->input_channels->at(0)->device();
        else
                io_device = arg->output_channels->at(0)->device();

        // open the device
        device = comedi_open(io_device);
        if (device == NULL) {
                Logger(Critical, "Unable to open device [%s].\n", io_device);
                pthread_exit((void *) retval);
        }
        Logger(Debug, "Successfully opened device [%s].\n", io_device);

        // get path of calibration file
        calibration_file = comedi_get_default_calibration_path(device);
        if (calibration_file == NULL) {
                Logger(Critical, "Unable to find default calibration file.\n");
                comedi_close(device);
                pthread_exit((void *) retval);
        }
        Logger(Debug, "The default calibration file is [%s].\n", calibration_file);

        // parse the calibration file
        calibration = comedi_parse_calibration_file(calibration_file);
        if (calibration == NULL) {
                Logger(Critical, "Unable to parse calibration file.\n");
                comedi_close(device);
                free(calibration_file);
                pthread_exit((void *) retval);
        }
        Logger(Debug, "Successfully parsed calibration file.\n");

        if (n_input_channels) {

                in_subdevice = arg->input_channels->at(0)->subdevice();
                in_chanlist = new uint[n_input_channels];
                in_converters = new comedi_polynomial_t[n_input_channels];
                // pack channel numbers and get a converter for each channel
                for (i=0; i<n_input_channels; i++) {
                        in_chanlist[i] = CR_PACK(arg->input_channels->at(i)->channel(),
                                                 arg->input_channels->at(i)->range(),
                                                 arg->input_channels->at(i)->reference());
                        ret = comedi_get_softcal_converter(in_subdevice,
                                                           arg->input_channels->at(i)->channel(),
                                                           arg->input_channels->at(i)->range(),
                                                           COMEDI_TO_PHYSICAL, calibration, &in_converters[i]);
                        if (ret < 0) {
                                Logger(Critical, "Unable to get converter for channel %d.\n", arg->input_channels->at(i)->channel());
                                goto end_io;
                        }
                        Logger(Debug, "Successfully obtained converter for channel %d.\n", arg->input_channels->at(i)->channel());
                }
                                
                if (comedi_get_subdevice_flags(device, in_subdevice) & SDF_LSAMPL)
                        in_bytes_per_sample = sizeof(lsampl_t);
                else
                        in_bytes_per_sample = sizeof(sampl_t);
                Logger(Debug, "Number of bytes per input sample: %d\n", in_bytes_per_sample);
                
                memset(&cmd, 0, sizeof(struct comedi_cmd_struct));
                ret = comedi_get_cmd_generic_timed(device, in_subdevice, &cmd,
                                n_input_channels, 1000000000*arg->dt);
                if (ret < 0) {
                        Logger(Critical, "Error in comedi_get_cmd_generic_timed.\n");
                        goto end_io;
                }
                cmd.convert_arg = 0;    // this value is wrong, but will be fixed by comedi_command_test
                cmd.chanlist    = in_chanlist;
                cmd.start_src   = TRIG_INT;
                cmd.stop_src    = TRIG_COUNT;
                cmd.stop_arg    = ceil(arg->tend/arg->dt);
                
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
        
                // allocate memory for reading the data
                input_buffer_length = ceil(arg->tend/arg->dt) * n_input_channels * in_bytes_per_sample;
                input_buffer = new char[input_buffer_length];
                Logger(Important, "The total size of the input buffer is %ld bytes (= %.2f Mb).\n",
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

                out_subdevice = arg->output_channels->at(0)->subdevice();
                out_chanlist = new uint[n_output_channels];
                out_converters = new comedi_polynomial_t[n_output_channels];
                // pack channel numbers and get a converter for each channel
                for (i=0; i<n_output_channels; i++) {
                        out_chanlist[i] = CR_PACK(arg->output_channels->at(i)->channel(),
                                                  arg->output_channels->at(i)->range(),
                                                  arg->output_channels->at(i)->reference());
                        ret = comedi_get_softcal_converter(out_subdevice,
                                                           arg->output_channels->at(i)->channel(),
                                                           arg->output_channels->at(i)->range(),
                                                           COMEDI_FROM_PHYSICAL, calibration, &out_converters[i]);
                        if (ret < 0) {
                                Logger(Critical, "Unable to get converter for channel %d.\n", arg->output_channels->at(i)->channel());
                                goto end_io;
                        }
                        Logger(Debug, "Successfully obtained converter for channel %d.\n", arg->output_channels->at(i)->channel());
                }
                                
                int flags = comedi_get_subdevice_flags(device, out_subdevice);
                if (flags & SDF_LSAMPL)
                        out_bytes_per_sample = sizeof(lsampl_t);
                else
                        out_bytes_per_sample = sizeof(sampl_t);
                Logger(Debug, "Number of bytes per output sample: %d\n", out_bytes_per_sample);
                
                memset(&cmd, 0, sizeof(struct comedi_cmd_struct));
                ret = comedi_get_cmd_generic_timed(device, out_subdevice, &cmd,
                                n_output_channels, 1000000000*arg->dt);
                if (ret < 0) {
                        Logger(Critical, "Error in comedi_get_cmd_generic_timed.\n");
                        goto end_io;
                }
                cmd.convert_arg = 0;    // this value is wrong, but will be fixed by comedi_command_test
                cmd.chanlist    = in_chanlist;
                cmd.start_src   = TRIG_INT;
                cmd.stop_src    = TRIG_COUNT;
                cmd.stop_arg    = ceil(arg->tend/arg->dt);
                
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
                output_buffer_length = ceil(arg->tend/arg->dt) * n_output_channels * out_bytes_per_sample;
                output_buffer = new char[output_buffer_length];
                Logger(Important, "The total size of the output buffer is %ld bytes (= %.2f Mb).\n",
                                output_buffer_length, (double) output_buffer_length/(1024*1024));

                // fill the buffer and preload it
                nsteps = ceil(arg->tend/arg->dt);
                double sample;
                sampl_t *ptr = (sampl_t*) output_buffer;
                lsampl_t *lptr = (lsampl_t*) output_buffer;
                for (i=0; i<nsteps; i++) {
                        for (j=0; j<n_output_channels; j++) {
                                sample = arg->output_channels->at(j)->at(i) * arg->output_channels->at(j)->conversionFactor();
                                if (sample > 0)
                                        fprintf(stdout, "%g = 0x%x\n", sample, comedi_from_physical(sample, &out_converters[j]));
                                if (flags & SDF_LSAMPL)
                                        *lptr = comedi_from_physical(sample, &out_converters[j]);
                                else
                                        *ptr = (sampl_t) comedi_from_physical(sample, &out_converters[j]);
                                ptr++;
                                lptr++;
                        }
                }

                bytes_written = write(comedi_fileno(device), (void *) output_buffer, output_buffer_length);
                if (bytes_written < 0) {
                        Logger(Critical, "Error on write: %s.\n", strerror(errno));
                }
                else if (bytes_written < output_buffer_length) {
                        fprintf(stderr, "failed to preload output buffer with %i bytes, is it too small?\n"
                                        "See the --write-buffer option of comedi_config\n", output_buffer_length);
                }
                Logger(Important, "Number of preloaded bytes: %d.\n", bytes_written);

                out_insn_data = 0;
                memset(&out_insn, 0, sizeof(out_insn));
                out_insn.insn = INSN_INTTRIG;
                out_insn.n = 1;
                out_insn.data = &out_insn_data;
                out_insn.subdev = out_subdevice;
        }

        in_retval = new int;
        out_retval = new int;
        *in_retval = *out_retval = ceil(arg->tend/arg->dt);

        if (n_input_channels) {
                // start the acquisition
                ret = comedi_do_insn(device, &in_insn);
                if (ret < 0)
                        Logger(Critical, "Unable to start the acquisition.\n");
                else
                        Logger(Debug, "Successfully started the acquisition.\n");
                for (i=0; i<n_input_channels; i++)
                        input_channels.push_back(arg->input_channels->at(i));
                in_data = new io_loop_data(device, in_subdevice, input_buffer, input_buffer_length, in_converters, &input_channels);
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
                        for (i=0; i<n_output_channels; i++)
                                output_channels.push_back(arg->output_channels->at(i));
                        out_data = new io_loop_data(device, out_subdevice, output_buffer, output_buffer_length, out_converters, &output_channels);
                        pthread_create(&out_loop_thrd, NULL, output_loop, (void *) out_data);
                }
        }

        if (n_input_channels) pthread_join(in_loop_thrd, (void **) &in_retval);
        if (n_output_channels && bytes_written < output_buffer_length) pthread_join(out_loop_thrd, (void **) &out_retval);

        arg->nsteps = MIN(*in_retval,*out_retval);

        delete in_retval;
        delete out_retval;
        delete in_data;
        delete out_data;

        *retval = 0;

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
        Logger(Debug, "Closed device [%s].\n", io_device);

        pthread_exit((void *) retval);
}

