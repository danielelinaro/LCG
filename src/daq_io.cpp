#include "daq_io.h"
#include "utils.h"

using namespace lcg;

char *cmd_src(int src,char *buf)
{
	buf[0]=0;

	if(src&TRIG_NONE)strcat(buf,"none|");
	if(src&TRIG_NOW)strcat(buf,"now|");
	if(src&TRIG_FOLLOW)strcat(buf, "follow|");
	if(src&TRIG_TIME)strcat(buf, "time|");
	if(src&TRIG_TIMER)strcat(buf, "timer|");
	if(src&TRIG_COUNT)strcat(buf, "count|");
	if(src&TRIG_EXT)strcat(buf, "ext|");
	if(src&TRIG_INT)strcat(buf, "int|");
#ifdef TRIG_OTHER
	if(src&TRIG_OTHER)strcat(buf, "other|");
#endif

	if(strlen(buf)==0){
		sprintf(buf,"unknown(0x%08x)",src);
	}else{
		buf[strlen(buf)-1]=0;
	}

	return buf;
}

void dump_cmd(FILE *out,comedi_cmd *cmd)
{
	char buf[100];

	fprintf(out,"subdev:      %d\n", cmd->subdev);
	fprintf(out,"flags:       %x\n", cmd->flags);

	fprintf(out,"start:      %-8s %d\n",
		cmd_src(cmd->start_src,buf),
		cmd->start_arg);

	fprintf(out,"scan_begin: %-8s %d\n",
		cmd_src(cmd->scan_begin_src,buf),
		cmd->scan_begin_arg);

	fprintf(out,"convert:    %-8s %d\n",
		cmd_src(cmd->convert_src,buf),
		cmd->convert_arg);

	fprintf(out,"scan_end:   %-8s %d\n",
		cmd_src(cmd->scan_end_src,buf),
		cmd->scan_end_arg);

	fprintf(out,"stop:       %-8s %d\n",
		cmd_src(cmd->stop_src,buf),
		cmd->stop_arg);
	
	fprintf(out,"chanlist_len: %d\n", cmd->chanlist_len);
	fprintf(out,"data_len: %d\n", cmd->data_len);
	
}


char *data_buffer = NULL;
size_t data_buffer_size = 0;

int allocate_io_memory(const std::vector<channel_opts*>& channels, double dt, double tend)
{
        size_t len = ceil(tend/dt) * channels.size();
        len = ceil(tend/dt);
}

int setup_io(const std::vector<channel_opts*>& channels, double dt, double tend)
{
        comedi_t *device;
        char *calibrationFile, *ptr;
        comedi_calibration_t *calibration;
        comedi_polynomial_t *converter;
        lsampl_t sample;
        unsigned int *in_chans, *in_chanlist, insn_data = 0;
        int i, ret, total, partial, subdevFlags, bytes_per_sample, priority;
        comedi_cmd cmd;
        comedi_insn insn;
        std::vector<channel_opts*> input_channels, output_channels;
        size_t n_input_channels, n_output_channels, input_buffer_length;
        char *input_buffer;

        // split the channels into input and output channels
        for (i=0; i<channels.size(); i++) {
                if (channels[i]->type == INPUT)
                        input_channels.push_back(channels[i]);
                else
                        output_channels.push_back(channels[i]);
        }
        n_input_channels = input_channels.size();
        n_output_channels = output_channels.size();

        // we assume that ALL channels have the same device
        // open the device
        char *input_device = input_channels[0]->device;
        device = comedi_open(input_device);
        if (device == NULL) {
                Logger(Critical, "Unable to open device [%s].\n", input_device);
                return -1;
        }
        Logger(Info, "Successfully opened device [%s].\n", input_device);

        // get path of calibration file
        calibrationFile = comedi_get_default_calibration_path(device);
        if (calibrationFile == NULL) {
                Logger(Critical, "Unable to find default calibration file.\n");
                comedi_close(device);
                return -1;
        }
        Logger(Info, "The default calibration file is [%s].\n", calibrationFile);

        // parse the calibration file
        calibration = comedi_parse_calibration_file(calibrationFile);
        if (calibration == NULL) {
                Logger(Critical, "Unable to parse calibration file.\n");
                comedi_close(device);
                free(calibrationFile);
                return -1;
        }
        Logger(Info, "Successfully parsed calibration file.\n");

        // pack channel numbers
        in_chans = new uint[n_input_channels];
        in_chanlist = new uint[n_input_channels];
        for (i=0; i<n_input_channels; i++) {
                in_chans[i] = input_channels[i]->channel;
                in_chanlist[i] = CR_PACK(input_channels[i]->channel, input_channels[i]->range, input_channels[i]->reference);
        }

        // get a converter for each channel
        converter = new comedi_polynomial_t[n_input_channels];
        for (i=0; i<n_input_channels; i++) {
                ret = comedi_get_softcal_converter(input_channels[i]->subdevice, input_channels[i]->channel, input_channels[i]->range,
                                COMEDI_TO_PHYSICAL, calibration, &converter[i]);
                if (ret < 0) {
                        Logger(Critical, "Unable to get converter for channel %d.\n", input_channels[i]->channel);
                        goto end_io;
                }
                Logger(Info, "Successfully obtained converter for channel %d.\n", input_channels[i]->channel);
        }
                        
        // get subdevice flags
        subdevFlags = comedi_get_subdevice_flags(device, input_channels[0]->subdevice);
        if (subdevFlags & SDF_LSAMPL)
                bytes_per_sample = sizeof(lsampl_t);
        else
                bytes_per_sample = sizeof(sampl_t);
        Logger(Important, "Number of bytes per sample: %d\n", bytes_per_sample);
        
        memset(&cmd, 0, sizeof(struct comedi_cmd_struct));
        ret = comedi_get_cmd_generic_timed(device, input_channels[0]->subdevice, &cmd,
                        n_input_channels, 1000000000*dt);
        if (ret < 0) {
                Logger(Critical, "Error in comedi_get_cmd_generic_timed.\n");
                goto end_io;
        }
        cmd.convert_arg = 0;    // this value is wrong, but will be fixed by comedi_command_test
        cmd.chanlist    = in_chanlist;
        cmd.start_src   = TRIG_INT;
        cmd.stop_src    = TRIG_COUNT;
        cmd.stop_arg    = ceil(tend/dt);
        
        // test and fix the command
        if (comedi_command_test(device, &cmd) < 0 &&
            comedi_command_test(device, &cmd) < 0) {
                Logger(Critical, "Unable to setup the command.\n");
                goto end_io;
        }
        Logger(Info, "Successfully setup the command.\n");
        Logger(Info, "-------------------------------\n");
        dump_cmd(stderr, &cmd);
        Logger(Info, "-------------------------------\n");

        // issue the command (acquisition doesn't start immediately)
        ret = comedi_command(device, &cmd);
        if (ret < 0) {
                Logger(Critical, "Unable to issue the command.\n");
                comedi_perror("comedi_command");
                goto end_io;
        }
        Logger(Info, "Successfully issued the command.\n");

        // allocate memory for reading the data
        input_buffer_length = ceil(tend/dt) * n_input_channels * bytes_per_sample;
        input_buffer = new char[input_buffer_length];
        Logger(Important, "The total size of the input buffer is %ld bytes (= %.0f Mb).\n",
                        input_buffer_length, ceil((double) input_buffer_length/(1024*1024)));

        // prepare the triggering instruction
        memset(&insn, 0, sizeof(insn));
        insn.insn = INSN_INTTRIG;
        insn.n = 1;
        insn.data = &insn_data;
        insn.subdev = input_channels[0]->subdevice;

        // start the acquisition
        ret = comedi_do_insn(device, &insn);
        if (ret < 0) {
                Logger(Critical, "Unable to start the acquisition.\n");
                goto end_io;
        }
        Logger(Info, "Successfully started the acquisition.\n");

        total = 0;
        while ((ret = read(comedi_fileno(device), input_buffer, input_buffer_length)) > 0) {
                total += ret;
                Logger(Info, "Read %d bytes from the board [%d/%d].\r", ret, total, input_buffer_length);
                for (i=0; i<ret/bytes_per_sample; i++) {
                        if (subdevFlags & SDF_LSAMPL)
                                sample = ((lsampl_t *) input_buffer)[i];
                        else
                                sample = ((sampl_t *) input_buffer)[i];
                        printf("%e\n", comedi_to_physical(sample, &converter[i%n_input_channels]));
                }
                //if (total >= DUR*NCHAN*SAMPLING_RATE*bytesPerSample)
                //        break;
        }
        Logger(Info, "\n");

        if (comedi_cancel(device, input_channels[0]->subdevice) == 0)
                Logger(Info, "Successfully stopped the command.\n");
        else
                Logger(Critical, "Unable to stop the command.\n");

end_io:
        delete in_chans;
        delete in_chanlist;
        delete converter;
        delete input_buffer;
        comedi_cleanup_calibration(calibration);
        Logger(Info, "Cleaned up the calibration.\n");
        free(calibrationFile);
        comedi_close(device);
        Logger(Info, "Closed device [%s].\n", input_device);

        return ret;
}

