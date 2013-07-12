#include "daq_io.h"
#include "utils.h"

using namespace lcg;

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

void* io_thread(void *in)
{
        comedi_t *device;
        char *calibration_file, *io_device;
        comedi_calibration_t *calibration;
        comedi_polynomial_t *in_converters, *out_converters;
        lsampl_t sample;
        uint *in_chanlist, *out_chanlist, in_subdevice, insn_data = 0;
        int i, ret, total, cnt, in_subdev_flags, in_bytes_per_sample;
        comedi_cmd cmd;
        comedi_insn insn;
        size_t n_input_channels, n_output_channels;
        size_t input_buffer_length;
        char *input_buffer;
        io_thread_arg *arg = static_cast<io_thread_arg*>(in);
        int *retval = new int;
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
                io_device = arg->input_channels->at(0)->device;
        else
                io_device = arg->output_channels->at(0)->device;

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

        // pack channel numbers and get a converter for each channel
        if (n_input_channels) {

                in_subdevice = arg->input_channels->at(0)->subdevice;
                in_chanlist = new uint[n_input_channels];
                in_converters = new comedi_polynomial_t[n_input_channels];
                for (i=0; i<n_input_channels; i++) {
                        in_chanlist[i] = CR_PACK(arg->input_channels->at(i)->channel,
                                                 arg->input_channels->at(i)->range,
                                                 arg->input_channels->at(i)->reference);
                        ret = comedi_get_softcal_converter(in_subdevice,
                                                           arg->input_channels->at(i)->channel,
                                                           arg->input_channels->at(i)->range,
                                                           COMEDI_TO_PHYSICAL, calibration, &in_converters[i]);
                        if (ret < 0) {
                                Logger(Critical, "Unable to get converter for channel %d.\n", arg->input_channels->at(i)->channel);
                                goto end_io;
                        }
                        Logger(Debug, "Successfully obtained converter for channel %d.\n", arg->input_channels->at(i)->channel);
                }
                                
                // get subdevice flags
                in_subdev_flags = comedi_get_subdevice_flags(device, in_subdevice);
                if (in_subdev_flags & SDF_LSAMPL)
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
                arg->data_length = ceil(arg->tend/arg->dt) * n_input_channels;
                input_buffer_length = arg->data_length * in_bytes_per_sample;
                arg->data = new double[arg->data_length];
                input_buffer = new char[input_buffer_length];
                Logger(Important, "The total size of the input buffer is %ld bytes (= %.2f Mb).\n",
                                input_buffer_length, (double) input_buffer_length/(1024*1024));
        }

        // prepare the triggering instruction
        memset(&insn, 0, sizeof(insn));
        insn.insn = INSN_INTTRIG;
        insn.n = 1;
        insn.data = &insn_data;
        insn.subdev = in_subdevice;

        // start the acquisition
        ret = comedi_do_insn(device, &insn);
        if (ret < 0) {
                Logger(Critical, "Unable to start the acquisition.\n");
                goto end_io;
        }
        Logger(Debug, "Successfully started the acquisition.\n");

        cnt = total = 0;
        while ((ret = read(comedi_fileno(device), input_buffer, input_buffer_length)) > 0) {
                total += ret;
                Logger(Debug, "Read %d bytes from the board [%d/%d].\r", ret, total, input_buffer_length);
                for (i=0; i<ret/in_bytes_per_sample; i++) {
                        if (in_subdev_flags & SDF_LSAMPL)
                                sample = ((lsampl_t *) input_buffer)[i];
                        else
                                sample = ((sampl_t *) input_buffer)[i];
                        arg->data[cnt++] = comedi_to_physical(sample, &in_converters[i%n_input_channels]) *
                                arg->input_channels->at(i%n_input_channels)->conversionFactor;
                }
        }
        *retval = 0;
        Logger(Debug, "\n");

        if (comedi_cancel(device, in_subdevice) == 0)
                Logger(Debug, "Successfully stopped the command.\n");
        else
                Logger(Critical, "Unable to stop the command.\n");

end_io:
        if (n_input_channels) {
                delete in_chanlist;
                delete in_converters;
                delete input_buffer;
        }
        comedi_cleanup_calibration(calibration);
        Logger(Debug, "Cleaned up the calibration.\n");
        free(calibration_file);
        comedi_close(device);
        Logger(Debug, "Closed device [%s].\n", io_device);

        pthread_exit((void *) retval);
}

