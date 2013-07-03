#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <math.h>

#include <vector>
#include <string>

#include "engine.h"
#include "common.h"
#include "types.h"
#include "utils.h"
#include "configuration.h"
#include "stimuli.h"
#include "h5rec.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

using namespace lcg;

//// GLOBAL VARIABLES - START
extern std::vector<Stimulus*> stimuli;
//// GLOBAL VARIABLES - END

typedef enum {DEFAULT, SPONTANEOUS, TRIGGERED} recording_mode;

/// PARSING OF COMMAND LINE ARGUMENTS - START

struct options {
        options() : tend(0), dt(0), iti(0), ibi(0),
                nTrials(0), nBatches(0), holdValue(0),
                stimulusFiles(), mode(DEFAULT), configFile(NULL) {}
        ~options() {
                if (configFile)
                        free(configFile);
        }
        double tend, dt;
        useconds_t iti, ibi;
        int nTrials, nBatches;
        double holdValue;
        std::vector<std::string> stimulusFiles;
        recording_mode mode;
        char *configFile;
};

static struct option longopts[] = {
        {"help", no_argument, NULL, 'h'},
        {"version", no_argument, NULL, 'v'},
        {"verbosity", required_argument, NULL, 'V'},
        {"frequency", required_argument, NULL, 'F'},
        {"iti", required_argument, NULL, 'i'},
        {"ibi", required_argument, NULL, 'I'},
        {"ntrials", required_argument, NULL, 'n'},
        {"nbatches", required_argument, NULL, 'N'},
        {"hold-value", required_argument, NULL, 'H'},
        {"duration", required_argument, NULL, 'd'},
        {"configfile", required_argument, NULL, 'c'},
        {"stimfile", required_argument, NULL, 'f'},
        {"stimdir", required_argument, NULL, 'D'},
        {NULL, 0, NULL, 0}
};

const char lcg_non_rt_usage_string[] =
        "This program performs voltage or current clamp experiments using stim-files.\n\n"
        "Usage: lcg non-rt [<options> ...]\n"
        "where options are:\n"
        "   -h, --help         Print this help message.\n"
        "   -v, --version      Print the program version.\n"
        "   -V, --verbosity    Verbosity level (0 for maximum, 4 for minimum verbosity).\n"
        "   -F, --frequency    Sampling rate (default 20000 Hz).\n"
        "   -i, --iti          Inter-trial interval.\n"
        "   -I, --ibi          Inter-batch interval (default same as inter-trial interval).\n"
        "   -n, --ntrials      Number of trials (how many times a stimulus is repeated, default 1).\n"
        "   -N, --nbatches     Number of batches (how many times a group of stimuli is repeated, default 1).\n"
        "   -H, --hold-value   Holding value (default 0).\n"
        "   -d, --duration     Duration of the recording (without a stimulus file).\n"
        "   -f, --stimfile     Stimulus file.\n"
        "   -c, --configfile   Configuration file.\n"
        "   -D, --stimdir      Directory containing the stimulus files.\n";

static void usage()
{
        printf("%s\n", lcg_non_rt_usage_string);
}

static void parse_args(int argc, char *argv[], options *opts)
{
        int ch;
        struct stat buf;
        double iti=-1, ibi=-1;
        DIR *dirp;
        struct dirent *dp;
        char filename[FILENAME_MAXLEN];
        // default values
        opts->nTrials = opts->nBatches = 1;
        opts->dt = 1./20000;
        while ((ch = getopt_long(argc, argv, "hvV:F:f:n:N:i:I:H:d:D:c:", longopts, NULL)) != -1) {
                switch(ch) {
                case 'h':
                        usage();
                        exit(0);
                case 'v':
                        printf("lcg non-rt version %s.\n", VERSION);
                        exit(0);
                case 'V':
                        if (atoi(optarg) < All || atoi(optarg) > Critical) {
                                Logger(Important, "The verbosity level must be between %d and %d.\n", All, Critical);
                                exit(1);
                        }
                        SetLoggingLevel(static_cast<LogLevel>(atoi(optarg)));
                        break;
                case 'F':
                        if (atof(optarg) <= 0) {
                                Logger(Critical, "The sampling frequency must be positive.\n");
                                exit(1);
                        }
                        opts->dt = 1./atof(optarg);
                        break;
                case 'n':
                        opts->nTrials = atoi(optarg);
                        if (opts->nTrials <= 0) {
                                Logger(Critical, "The number of trials must be greater than zero.\n");
                                exit(1);
                        }
                        break;
                case 'N':
                        opts->nBatches = atoi(optarg);
                        if (opts->nBatches <= 0) {
                                Logger(Critical, "The number of batches must be greater than zero.\n");
                                exit(1);
                        }
                        break;
                case 'i':
                        iti = atof(optarg);
                        if (iti < 0) {
                                Logger(Critical, "The inter-trial interval must be non-negative.\n");
                                exit(1);
                        }
                        opts->iti = (useconds_t) (1e6 * iti);
                        break;
                case 'I':
                        ibi = atof(optarg);
                        if (ibi < 0) {
                                Logger(Critical, "The inter-batch interval must be not negative.\n");
                                exit(1);
                        }
                        opts->ibi = (useconds_t) (1e6 * ibi);
                        break;
                case 'H':
                        opts->holdValue = atof(optarg);
                        break;
                case 'd':
                        if (opts->stimulusFiles.size() != 0) {
                                Logger(Critical, "You cannot specify -d and either -f or -D simultaneously.\n");
                                exit(1);
                        }
                        opts->tend = atof(optarg);
                        if (opts->tend <= 0) {
                                Logger(Critical, "The duration of the recording must be positive.\n");
                                exit(1);
                        }
                        opts->mode = SPONTANEOUS;
                        break;
                case 'D':
                        if (opts->tend != 0) {
                                Logger(Critical, "You cannot specify -d and either -f or -D simultaneously.\n");
                                exit(1);
                        }
                        if (opts->stimulusFiles.size() != 0) {
                                Logger(Critical, "You cannot specify -f and -D simultaneously.\n");
                                exit(1);
                        }
                        dirp = opendir(optarg);
                        if (dirp == NULL) {
                                Logger(Critical, "%s: %s.\n", optarg, strerror(errno));
                                exit(1);
                        }
                        while ((dp = readdir(dirp)) != NULL) {
                                if (dp->d_name[0] != '.') {
                                        sprintf(filename, "%s/%s", optarg, dp->d_name);
                                        opts->stimulusFiles.push_back(filename);
                                }
                        }
                        closedir(dirp);
                        break;
                case 'f':
                        if (opts->tend != 0) {
                                Logger(Critical, "You cannot specify -d and either -f or -D simultaneously.\n");
                                exit(1);
                        }
                        if (opts->stimulusFiles.size() != 0) {
                                Logger(Critical, "You cannot specify -f and -D simultaneously.\n");
                                exit(1);
                        }
                        if (stat(optarg, &buf) == -1) {
                                Logger(Critical, "%s: %s.\n", optarg, strerror(errno));
                                exit(1);
                        }
                        opts->stimulusFiles.push_back(optarg);
                        break;
                case 'c':
                        if (stat(optarg, &buf) == -1) {
                                Logger(Critical, "%s: %s.\n", optarg, strerror(errno));
                                exit(1);
                        }
                        opts->configFile = (char *) malloc(FILENAME_MAXLEN*sizeof(char));
                        strncpy(opts->configFile, optarg, FILENAME_MAXLEN);
                        break;
                default:
                        Logger(Critical, "Enter 'lcg help non-rt' for help on how to use this program.\n");
                        exit(1);
                }
        }
        if (iti < 0 && (opts->nTrials > 1 || opts->stimulusFiles.size() > 1)) {
                Logger(Critical, "You must specify the inter-trial interval (-i switch).\n");
                exit(1);
        }
        if (ibi < 0)
                opts->ibi = opts->iti;
}

/// PARSING OF COMMAND LINE ARGUMENTS - END

/*
int run_trial(const io_options *input, const io_options *output)
{
        int i, id, nsteps;
        ChunkedH5Recorder rec;
        double_dict pars;
        allocate_stimuli(1);
        nsteps = ceil(trial_duration / GetGlobalDt());
        for (i=0, id=0; i<input->nchan; i++, id++)
                rec.addRecord(id, "AnalogInput", input->units, nsteps, pars);
        for (i=0; i<output->nchan; i++, id++) {
                rec.addRecord(id, "Waveform", output->units, nsteps, pars);
                rec.writeRecord(id, stimuli[i], nsteps);
        }
        Logger(Important, "Trial duration: %g seconds.\n", trial_duration);
        free_stimuli(1);
        return 0;
}
*/

int main(int argc, char *argv[])
{
        if (!SetupSignalCatching()) {
                Logger(Critical, "Unable to setup signal catching functionalities.\n");
                exit(1);
        }

        options opts;
        //io_options input_opts, output_opts;
        std::vector<channel_opts*> channels;
        int i, j, k;
        int cnt, total;
        int err;

        SetLoggingLevel(Info);
        
	parse_args(argc, argv, &opts);

        SetGlobalDt(opts.dt);

        err = parse_configuration_file(opts.configFile, channels);
        if (err) {
                Logger(Critical, "Unable to parse the configuration file.\n");
                goto endMain;
        }

        Logger(Info, "Number of batches: %d.\n", opts.nBatches);
        Logger(Info, "Number of trials: %d.\n", opts.nTrials);
        Logger(Info, "Inter-trial interval: %g sec.\n", (double) opts.iti * 1e-6);
        Logger(Info, "Holding current: %g pA.\n", opts.holdValue);
        Logger(Info, "Inter-batch interval: %g sec.\n", (double) opts.ibi * 1e-6);

        cnt = 1;
	total = opts.nBatches*opts.stimulusFiles.size()*opts.nTrials;
	for (i=0; i<opts.nBatches; i++) {
	        for (j=0; j<opts.stimulusFiles.size(); j++) {
                        //strncpy(stimulus_files[0], opts.stimulusFiles[j].c_str(), FILENAME_MAXLEN);
			for (k=0; k<opts.nTrials; k++, cnt++) {
                                Logger(Important, "Trial: %d of %d.\n", cnt, total);
                                //err = run_trial(&input_opts, &output_opts);
                                err = 0;
                                if (err || KILL_PROGRAM())
                                        goto endMain;
                                if (k != opts.nTrials-1)
                                        usleep(opts.iti);
                        }
                        if (j != opts.stimulusFiles.size()-1)
                                usleep(opts.iti);
		}
		if (i != opts.nBatches-1)
                        usleep(opts.ibi);
	}

endMain:
        return err;
}

