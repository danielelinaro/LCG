#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>

#include <vector>
#include <string>
#include <iostream>
#include <sstream>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

#include "types.h"
#include "utils.h"
#include "entity.h"
#include "engine.h"
#include "waveform.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define CONFIG_FILE ".cclamprc"

using boost::property_tree::ptree;
using namespace lcg;

typedef enum {DEFAULT, SPONTANEOUS, TRIGGERED} recording_mode;

struct options {
        options() : tend(0), dt(0), iti(0), ibi(0),
                nTrials(0), nBatches(0), holdValue(0),
                stimulusFiles(), mode(DEFAULT) {}
        double tend, dt;
        useconds_t iti, ibi;
        int nTrials, nBatches;
        double holdValue;
        std::vector<std::string> stimulusFiles;
        recording_mode mode;
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
        {"stimfile", required_argument, NULL, 'f'},
        {"stimdir", required_argument, NULL, 'D'},
        {NULL, 0, NULL, 0}
};

const char lcg_vcclamp_usage_string[] =
        "This program performs voltage or current clamp experiments using stim-files.\n\n"
        "Usage: lcg vcclamp [<options> ...]\n"
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
        "   -D, --stimdir      Directory containing the stimulus files.\n";

static void usage()
{
        printf("%s\n", lcg_vcclamp_usage_string);
}

static int write_default_configuration_file()
{
        char *home, configFile[60] = {0};
        FILE *fid;
        home = getenv("HOME");
        if (home == NULL) {
                Logger(Critical, "Unable to get HOME environment variable.\n");
                return -1;
        }
        sprintf(configFile, "%s/%s", home, CONFIG_FILE);
        fid = fopen(configFile, "w");
        if (fid == NULL) {
                Logger(Critical, "Unable to open [%s].\n", configFile);
                return -1;
        }

        fprintf(fid, "[AnalogInput0]\n");
        fprintf(fid, "device = %s\n", getenv("COMEDI_DEVICE"));
        fprintf(fid, "range = %s\n", getenv("DAQ_RANGE"));
        fprintf(fid, "subdevice = %s\n", getenv("AI_SUBDEVICE"));
        fprintf(fid, "channel = %s\n", getenv("AI_CHANNEL"));
        fprintf(fid, "conversionFactor = %s\n", getenv("AI_CONVERSION_FACTOR"));
        fprintf(fid, "reference = %s\n", getenv("GROUND_REFERENCE"));
        fprintf(fid, "units = %s\n", getenv("INPUT_UNITS"));
        fprintf(fid, "\n");
        fprintf(fid, "[AnalogOutput0]\n");
        fprintf(fid, "device = %s\n", getenv("COMEDI_DEVICE"));
        fprintf(fid, "range = %s\n", getenv("DAQ_RANGE"));
        fprintf(fid, "subdevice = %s\n", getenv("AO_SUBDEVICE"));
        fprintf(fid, "channel = %s\n", getenv("AO_CHANNEL"));
        fprintf(fid, "conversionFactor = %s\n", getenv("AO_CONVERSION_FACTOR"));
        fprintf(fid, "reference = %s\n", getenv("GROUND_REFERENCE"));
        fprintf(fid, "units = %s\n", getenv("OUTPUT_UNITS"));

        fclose(fid);

        Logger(Critical, "Successfully saved default configuration file in [%s].\n", configFile);
        return 0;
}

static void parse_args(int argc, char *argv[], options *opts)
{
        int ch;
        struct stat buf;
        double iti=-1, ibi=-1;
        DIR *dirp;
        struct dirent *dp;
        // default values
        opts->nTrials = opts->nBatches = 1;
        opts->dt = 1./20000;
        while ((ch = getopt_long(argc, argv, "hvV:F:f:n:N:i:I:H:d:D:", longopts, NULL)) != -1) {
                switch(ch) {
                case 'h':
                        usage();
                        exit(0);
                case 'v':
                        printf("lcg vcclamp version %s.\n", VERSION);
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
                        if (atof(optarg) < 0) {
                                Logger(Critical, "The inter-batch interval must be not negative.\n");
                                exit(1);
                        }
                        opts->ibi = (useconds_t) (1e6 * atof(optarg));
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
                                if (dp->d_name[0] != '.')
                                        opts->stimulusFiles.push_back(dp->d_name);
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
                default:
                        Logger(Critical, "Enter 'lcg help vcclamp' for help on how to use this program.\n");
                        exit(1);
                }
        }
        if (opts->tend == 0 && opts->stimulusFiles.size() == 0) {
                SetLoggingLevel(Info);
                Logger(Info, "You must specify one of the following three options:\n"
                                 "   -d, --duration     Duration of the recording (without a stimulus file).\n"
                                 "   -f, --stimfile     Stimulus file.\n"
                                 "   -D, --stimdir      Directory containing the stimulus files.\n");
                exit(1);
        }
        if (iti < 0 && (opts->nTrials > 1 || opts->stimulusFiles.size() > 1)) {
                Logger(Critical, "You must specify the inter-trial interval (-i switch).\n");
                exit(1);
        }
        if (ibi < 0)
                opts->ibi = opts->iti;
}

static int parse_configuration_file(std::vector<Entity*>& entities, const options *opts)
{
        ptree pt;
        string_dict parameters;
        struct stat buf;
        char *home, configFile[60] = {0};

        home = getenv("HOME");
        if (home == NULL) {
                Logger(Critical, "Unable to get HOME environment variable.\n");
                return -1;
        }
        sprintf(configFile, "%s/%s", home, CONFIG_FILE);

        if (stat(configFile, &buf) == -1) {
                Logger(Critical, "%s: %s\n", configFile, strerror(errno));
                return -1;
        }

        read_ini(configFile, pt);

        try {
                parameters["id"] = "0";
                parameters["compress"] = "true";
                entities.push_back( EntityFactory("H5Recorder", parameters) );
                
                if (opts->mode == SPONTANEOUS) {
		        Logger(Debug, "Not using a Waveform.\n");
                }
                else if (opts->mode == TRIGGERED) {
                        Logger(Critical, "TRIGGERED mode: don't know what to do...\n");
                        exit(1);
                }
                else {
		        parameters.clear();
		        parameters["id"] = "1";
			parameters["units"] = "pA";
			entities.push_back( EntityFactory("Waveform", parameters) );
		}

                int AI_cnt = 0;
                while (true) {
                        std::stringstream id;
                        id << entities.back()->id() + 1;
                        parameters.clear();
                        parameters["id"] = id.str();
                        try {
                                char str[64];
                                sprintf(str, "AnalogInput%d.device", AI_cnt);
                                parameters["deviceFile"] = pt.get<std::string>(str);
                                sprintf(str, "AnalogInput%d.range", AI_cnt);
                                parameters["range"] = pt.get<std::string>(str);
                                sprintf(str, "AnalogInput%d.subdevice", AI_cnt);
                                parameters["inputSubdevice"] = pt.get<std::string>(str);
                                sprintf(str, "AnalogInput%d.channel", AI_cnt);
                                parameters["readChannel"] = pt.get<std::string>(str);
                                sprintf(str, "AnalogInput%d.conversionFactor", AI_cnt);
                                parameters["inputConversionFactor"] = pt.get<std::string>(str);
                                sprintf(str, "AnalogInput%d.reference", AI_cnt);
                                parameters["reference"] = pt.get<std::string>(str);
                                sprintf(str, "AnalogInput%d.units", AI_cnt);
                                parameters["units"] = pt.get<std::string>(str);
                        } catch(...) {
                                break;
                        }
                        entities.push_back( EntityFactory("AnalogInput", parameters) );
                        AI_cnt++;
                }
				
                if (opts->mode == SPONTANEOUS) {
		        Logger(Debug, "Connecting the analog inputs [%d to %d] to the recorder.\n",
			entities.size()-AI_cnt, entities.size());
			for (int i=entities.size()-AI_cnt; i<entities.size(); i++)
			        entities[i]->connect(entities[0]);
                }
                else {
		        std::stringstream id;
			id << entities.back()->id() + 1;
		        parameters.clear();
			parameters["id"] = id.str();
			parameters["deviceFile"] = pt.get<std::string>("AnalogOutput0.device");
			parameters["outputSubdevice"] = pt.get<std::string>("AnalogOutput0.subdevice");
			parameters["writeChannel"] = pt.get<std::string>("AnalogOutput0.channel");
			parameters["outputConversionFactor"] = pt.get<std::string>("AnalogOutput0.conversionFactor");
			parameters["reference"] = pt.get<std::string>("AnalogOutput0.reference");
			parameters["units"] = pt.get<std::string>("AnalogOutput0.units");

			entities.push_back( EntityFactory("AnalogOutput", parameters) );
			Logger(Debug, "Connecting the stimulus to the recorder.\n");
			entities[1]->connect(entities[0]);
			Logger(Debug, "Connecting the stimulus to the analog output.\n");
			entities[1]->connect(entities.back());
			
			Logger(Debug, "Connecting the analog inputs [%d to %d] to the recorder.\n",
						entities.size()-(AI_cnt+1), entities.size()-1);
			for (int i=entities.size()-(AI_cnt+1); i<entities.size()-1; i++)
				entities[i]->connect(entities[0]);

			if(opts->holdValue != 0.0) {
				Logger(Debug,"Using %f as a holding value.\n", opts->holdValue);
				std::stringstream holdValue_str;
				holdValue_str << opts->holdValue;
				std::stringstream id;
				id << entities.back()->id() + 1;
				parameters.clear();
				parameters["id"] = id.str();
				parameters["units"] = "pA";
				parameters["value"] = holdValue_str.str();
				entities.push_back( EntityFactory("Constant", parameters) );
				Logger(Debug, "Connecting the constant to the [%d to %d] to the recorder.\n",
						entities.size(), 0);
				entities.back()->connect(entities[0]);
				Logger(Debug, "Connecting the constant to the analog output.\n");
				entities.back()->connect(entities[entities.size()-2]);
			}   
		}   	
        } catch (const char *msg) {
                Logger(Critical, "Error: %s\n", msg);
                return -1;
        }
        Logger(Debug, "Successfully parsed configuration file [%s].\n", configFile);
        return 0;
}

int main(int argc, char *argv[])
{
#ifndef HAVE_LIBCOMEDI
        Logger(Critical, "This program requires Comedi.\n");
        exit(0);
#else
        if (!SetupSignalCatching()) {
                Logger(Critical, "Unable to setup signal catching functionalities.\n");
                exit(1);
        }

        std::vector<Entity*> entities;
        lcg::generators::Waveform *stimulus;
        options opts;
        int i, j, k, cnt, total, retval = 0;

        SetLoggingLevel(Info);
        
	parse_args(argc, argv, &opts);

        if (parse_configuration_file(entities, &opts) != 0) {
                write_default_configuration_file();
                parse_configuration_file(entities, &opts);
        }

        if (opts.mode == DEFAULT) {
                // we need to find the Waveform object
	        for (i=0; i<entities.size(); i++) {
		        if ((stimulus = dynamic_cast<lcg::generators::Waveform*>(entities[i])) != NULL)
		            break;
		}
		if (i == entities.size()) {
			Logger(Critical, "No stimulus present.\n");
			retval = 1;
			goto endMain;
		}
	}

        SetGlobalDt(opts.dt);

        Logger(Info, "Number of batches: %d.\n", opts.nBatches);
        Logger(Info, "Number of trials: %d.\n", opts.nTrials);
        Logger(Info, "Inter-trial interval: %g sec.\n", (double) opts.iti * 2e-6);
        Logger(Info, "Holding current: %g pA.\n", opts.holdValue);
        Logger(Info, "Inter-batch interval: %g sec.\n", (double) opts.ibi * 1e-6);

        bool success;
        cnt = 1;
        if (opts.mode == SPONTANEOUS) {
		total = opts.nBatches*opts.nTrials;
		for (i=0; i<opts.nBatches; i++) {
			for (k=0; k<opts.nTrials; k++, cnt++) {
                                Logger(Important, "Trial: %d of %d.\n", cnt, total);
                                success = Simulate(entities, opts.tend);
                                if (!success || KILL_PROGRAM())
                                        goto endMain;
                                if (k != opts.nTrials-1)
                                        usleep(opts.iti);
                        }
			if (i != opts.nBatches-1)
                                usleep(opts.ibi);
		}
        }
        else if (opts.mode == TRIGGERED) {
                // what should we do here?
                Logger(Critical, "ops.\n");
        }
        else {
		total = opts.nBatches*opts.stimulusFiles.size()*opts.nTrials;
		for (i=0; i<opts.nBatches; i++) {
		        for (j=0; j<opts.stimulusFiles.size(); j++) {
                                stimulus->setStimulusFile(opts.stimulusFiles[j].c_str());
				for (k=0; k<opts.nTrials; k++, cnt++) {
                                        Logger(Important, "Trial: %d of %d.\n", cnt, total);
                                        success = Simulate(entities, stimulus->duration());
                                        if (!success || KILL_PROGRAM())
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
	}

endMain:
        for (uint i=0; i<entities.size(); i++)
                delete entities[i];

        return retval;
#endif
}

