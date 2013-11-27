#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <getopt.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <comedilib.h>
#include "common.h"
#include "types.h"
#include "utils.h"
using namespace lcg;

struct options {
        options() {
                strncpy(device, getenv("COMEDI_DEVICE"), FILENAME_MAXLEN);
                subdevice = atoi(getenv("AO_SUBDEVICE"));
                channel = -1;   // all channels
                reference = strncmp(getenv("GROUND_REFERENCE"), "NRSE", 5) ? AREF_GROUND : AREF_COMMON;
                value = 0.0;
                factor = atof(getenv("AO_CONVERSION_FACTOR_CC"));
        }
        char device[FILENAME_MAXLEN];
        uint subdevice, reference;
        int channel;
        double value, factor;
};

static struct option longopts[] = {
        {"help", no_argument, NULL, 'h'},
        {"device", required_argument, NULL, 'd'},
        {"subdevice", required_argument, NULL, 's'},
        {"channel", required_argument, NULL, 'c'},
        {"reference", required_argument, NULL, 'r'},
        {"value", required_argument, NULL, 'v'},
        {"factor", required_argument, NULL, 'f'},
        {NULL, 0, NULL, 0}
};

const char lcg_output_usage_string[] =
        "This program outputs a constant value to a certain channel of the DAQ board.\n\n"
        "Usage: lcg output [<options> ...]\n"
        "where options are:\n"
        "   -h, --help       Print this help message and exit.\n"
        "   -v, --value      Constant value to output.\n"
        "   -f, --factor     Conversion factor.\n"
        "   -d, --device     Path of the DAQ device.\n"
        "   -s, --subdevice  Subdevice number.\n"
        "   -c, --channel    Channel number (if not specified, all channels are reset).\n"
        "   -r, --reference  Ground reference (either GRSE or NRSE).\n";

void usage()
{
        printf("%s\n", lcg_output_usage_string);
}

void parse_args(int argc, char *argv[], options *opts)
{
        int ch;
        struct stat buf;
        while ((ch = getopt_long(argc, argv, "hd:s:c:r:v:f:", longopts, NULL)) != -1) {
                switch(ch) {
                case 'h':
                        usage();
                        exit(0);
                case 's':
                        opts->subdevice = atoi(optarg);
                        break;
                case 'c':
                        opts->channel = atoi(optarg);
                        break;
                case 'r':
                        if (strncmp(optarg, "NRSE", 5) == 0) {
                                opts->reference = AREF_GROUND;
                        }
                        else if (strncmp(optarg, "GRSE", 5) == 0) {
                                opts->reference = AREF_COMMON;
                        }
                        else {
                                Logger(Critical, "%s: unknown reference mode.\n", optarg);
                                exit(1);
                        }
                        break;
                case 'd':
                        if (stat(optarg, &buf) == -1) {
                                Logger(Critical, "%s: %s\n", optarg, strerror(errno));
                                exit(1);
                        }
                        strncmp(opts->device, optarg, FILENAME_MAXLEN);
                        break;
                case 'v':
                        opts->value = atof(optarg);
                        break;
                case 'f':
                        opts->factor = atof(optarg);
                        if (opts->factor <= 0.) {
                                Logger(Critical, "The conversion factor must be positive.\n");
                                exit(1);
                        }
                        break;
                default:
                        Logger(Critical, "Enter 'lcg help output' for help on how to use this program.\n");
                        exit(1);
                }
        }
}

int main(int argc, char *argv[])
{
        FILE *fid;
        comedi_t *device;
        char *calibration_file;
        comedi_calibration_t *calibration;
        comedi_polynomial_t converter;
        options opts;

        parse_args(argc, argv, &opts);

        Logger(Info, "Outputting %g on device [%s], subdevice [%d], channel [%d].\n", opts.value, opts.device, opts.subdevice, opts.channel);

        device = comedi_open(opts.device);
        calibration_file = comedi_get_default_calibration_path(device);
        calibration = comedi_parse_calibration_file(calibration_file);
        comedi_get_softcal_converter(opts.subdevice, opts.channel, 0, COMEDI_FROM_PHYSICAL, calibration, &converter);
        comedi_data_write(device, opts.subdevice, opts.channel, 0, opts.reference, comedi_from_physical(opts.value*opts.factor, &converter));

        comedi_cleanup_calibration(calibration);
        free(calibration_file);
        comedi_close(device);

        fid = fopen(LOGFILE,"w");
        if (fid != NULL) {
                fprintf(fid, "%lf", opts.value);
                fclose(fid);
        }

        return 0;
}

