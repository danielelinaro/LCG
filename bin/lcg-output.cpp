#ifdef ANALOG_IO

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
                channel = atoi(getenv("AO_CHANNEL"));
                reference = strncmp(getenv("GROUND_REFERENCE"), "NRSE", 5) ? AREF_GROUND : AREF_COMMON;
                value = 0.0;
                factor = -1.;
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
        {"voltage-clamp", no_argument, NULL, 'v'},
        {NULL, 0, NULL, 0}
};

const char lcg_output_usage_string[] =
        "This program outputs a constant value to a given channel of the DAQ board.\n\n"
        "Usage: lcg output [<options> ...] value\n\n"
        "where options are:\n\n"
        "   -h, --help           Print this help message and exit.\n"
        "   -d, --device         Path of the DAQ device (default %s).\n"
        "   -s, --subdevice      Subdevice number (default %s).\n"
        "   -c, --channel        Channel number (default %s).\n"
        "   -r, --reference      Ground reference (GRSE or NRSE, default %s).\n"
        "   -f, --factor         Conversion factor (default %s in current clamp mode, %s in voltage clamp mode).\n"
        "   -v, --voltage-clamp  Use voltage clamp conversion factor.\n";

void usage()
{
        printf(lcg_output_usage_string, getenv("AO_CONVERSION_FACTOR_CC"), getenv("AO_CONVERSION_FACTOR_VC"),
                getenv("COMEDI_DEVICE"), getenv("AO_SUBDEVICE"), getenv("AO_CHANNEL"), getenv("GROUND_REFERENCE"));
}

void parse_args(int argc, char *argv[], options *opts)
{
        int ch;
        struct stat buf;
        bool vclamp = false;
        opts->factor = -1.;
        while ((ch = getopt_long(argc, argv, "hd:s:c:r:f:v", longopts, NULL)) != -1) {
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
                case 'f':
                        opts->factor = atof(optarg);
                        if (opts->factor <= 0.) {
                                Logger(Critical, "The conversion factor must be positive.\n");
                                exit(1);
                        }
                        break;
                case 'v':
                        vclamp = true;
                        break;
                default:
                        Logger(Critical, "Enter 'lcg help output' for help on how to use this program.\n");
                        exit(1);
                }
        }
        if (opts->factor < 0.) {
                if (vclamp)
                        opts->factor = atof(getenv("AO_CONVERSION_FACTOR_VC"));
                else
                        opts->factor = atof(getenv("AO_CONVERSION_FACTOR_CC"));
        }
        else if (vclamp) {
                Logger(Important, "Ignoring voltage clamp option since you explicitly passed a conversion factor.\n"); 
        }
        if (optind != argc-1) {
                Logger(Critical, "You must specify a value to output.\n");
                exit(1);
        }
        opts->value = atof(argv[optind]);
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

        Logger(Info, "Outputting %g on device [%s], subdevice [%d], channel [%d], conversion factor = %g.\n",
                        opts.value, opts.device, opts.subdevice, opts.channel, opts.factor);

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

#else

#include "utils.h"
using namespace lcg;

int main() {
        Logger(Critical, "This program requires a working installation of Comedi.\n");
        return -1;
}

#endif

