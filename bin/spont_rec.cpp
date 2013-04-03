#include <stdlib.h>
#include <sys/types.h>

#include <vector>
#include <string>
#include <iostream>
#include <sstream>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
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

#define CONFIG_FILE ".spont_recrc"
#define SPONT_REC_VERSION "0.1"

namespace po = boost::program_options;
namespace fs = boost::filesystem;
using boost::property_tree::ptree;
using namespace dynclamp;

// Spontaneous recorder options
struct SRoptions {
		useconds_t iti, ibi;
        uint nTrials, nBatches;
        double dt;
		double tend;
};

bool writeDefaultConfigurationFile()
{
        char *home, configFile[60] = {0};
        FILE *fid;
        home = getenv("HOME");
        if (home == NULL) {
                Logger(Critical, "Unable to get HOME environment variable.\n");
                return false;
        }
        sprintf(configFile, "%s/%s", home, CONFIG_FILE);
        fid = fopen(configFile, "w");
        if (fid == NULL) {
                Logger(Critical, "Unable to open [%s].\n", configFile);
                return false;
        }

        fprintf(fid, "[AnalogInput0]\n");
        fprintf(fid, "device = /dev/comedi0\n");
        fprintf(fid, "range = [-10,+10]\n");
        fprintf(fid, "subdevice = 0\n");
        fprintf(fid, "channel = 0\n");
        fprintf(fid, "conversionFactor = 100\n");
        fprintf(fid, "reference = GRSE\n");
        fprintf(fid, "units = mV\n");
        fprintf(fid, "\n");

        fclose(fid);

        Logger(Info, "Successfully saved default configuration file in [%s].\n", configFile);
        return true;
}

void parseArgs(int argc, char *argv[], SRoptions *opt)
{
        double iti, freq, dur;
        int verbosity;
        uint nTrials;

        std::string caption("Spontaneous recorder\nAllowed options");
        po::options_description description(caption);

		po::variables_map options;

        try {
                char msg[100];
                sprintf(msg, "select verbosity level (%d for maximum, %d for minimum verbosity)", All, Critical);
                description.add_options()
                        ("help,h", "print help message")
                        ("version,v", "print version number")
                        ("verbosity,V", po::value<int>(&verbosity)->default_value(Info), msg)
                        ("iti,i", po::value<double>(&iti)->default_value(0.25), "specify inter-trial interval (default: 0.25 sec)")
                        ("ntrials,n", po::value<uint>(&nTrials)->default_value(1), "specify the number of trials (number of repetitions per batch)")
                        ("frequency,F", po::value<double>(&freq)->default_value(20000), "specify the sampling frequency")
                        ("duration,d", po::value<double>(&dur)->default_value(60), "specify the duration of the recording");

                po::store(po::parse_command_line(argc, argv, description), options);
                po::notify(options);    

                if (options.count("help")) {
                        std::cout << description << "\n";
                        exit(0);
                }

                if (options.count("version")) {
					std::cout << fs::path(argv[0]).filename() << " version " << SPONT_REC_VERSION << std::endl;
                        exit(0);
                }

                if (freq <= 0) {
                        std::cerr << "The sampling frequency must be positive.\n";
                        exit(1);
                }

                if (dur <= 0) {
                        std::cerr << "The duration must be positive.\n";
                        exit(1);
                }

                if (verbosity < All || verbosity > Critical) {
                        Logger(Important, "The verbosity level must be between %d and %d.\n", All, Critical);
                        exit(1);
                }

                SetLoggingLevel(static_cast<LogLevel>(verbosity));

                opt->dt = 1.0 / freq;
                opt->tend = dur;
                opt->iti = (useconds_t) (iti * 1e6);
                opt->ibi = (useconds_t) (0.0 * 1e6);
                opt->nTrials = nTrials;
                opt->nBatches = 1;

        } catch (std::exception e) {
                std::cout << e.what() << std::endl;
                exit(2);
        }

}

bool parseConfigurationFile(std::vector<Entity*>& entities)
{
        ptree pt;
        string_dict parameters;
        char *home, configFile[60] = {0};

        home = getenv("HOME");
        if (home == NULL) {
                Logger(Critical, "Unable to get HOME environment variable.\n");
                return false;
        }
        sprintf(configFile, "%s/%s", home, CONFIG_FILE);

        if (! fs::exists(configFile)) {
                Logger(Critical, "Configuration file [%s] not found.\n", configFile);
                return false;
        }

        read_ini(configFile, pt);

        try {
                parameters["id"] = "0";
                parameters["compress"] = "true";
                entities.push_back( EntityFactory("H5Recorder", parameters) );
                
                int cnt = 0;
                while (true) {
                        std::stringstream id;
                        id << entities.back()->id() + 1;
                        parameters.clear();
                        parameters["id"] = id.str();
                        try {
                                char str[64];
                                sprintf(str, "AnalogInput%d.device", cnt);
                                parameters["deviceFile"] = pt.get<std::string>(str);
                                sprintf(str, "AnalogInput%d.range", cnt);
                                parameters["range"] = pt.get<std::string>(str);
                                sprintf(str, "AnalogInput%d.subdevice", cnt);
                                parameters["inputSubdevice"] = pt.get<std::string>(str);
                                sprintf(str, "AnalogInput%d.channel", cnt);
                                parameters["readChannel"] = pt.get<std::string>(str);
                                sprintf(str, "AnalogInput%d.conversionFactor", cnt);
                                parameters["inputConversionFactor"] = pt.get<std::string>(str);
                                sprintf(str, "AnalogInput%d.reference", cnt);
                                parameters["reference"] = pt.get<std::string>(str);
                                sprintf(str, "AnalogInput%d.units", cnt);
                                parameters["units"] = pt.get<std::string>(str);
                        } catch(...) {
                                break;
                        }
                        entities.push_back( EntityFactory("AnalogInput", parameters) );
                        cnt++;
                }

                std::stringstream id;
                id << entities.back()->id() + 1;
                parameters.clear();
                Logger(Debug, "Connecting the analog inputs to the recorder.\n");
                for (int i=1; i<entities.size(); i++)
                        entities[i]->connect(entities[0]);

        } catch (const char *msg) {
                Logger(Critical, "Error: %s\n", msg);
                return false;
        }

        Logger(Debug, "Successfully parsed configuration file [%s].\n", configFile);
        return true;
}

int main(int argc, char *argv[])
{
#ifndef HAVE_LIBCOMEDI

        Logger(Critical, "This program requires Comedi.\n");
        exit(0);

#else
        if (!SetupSignalCatching()) {
                Logger(Critical, "Unable to setup signal catching functionalities. Aborting.\n");
                exit(1);
        }

        std::vector<Entity*> entities;
        SRoptions opt;
        int i, j, k, cnt, total, retval = 0;

        SetLoggingLevel(Info);

        if (! parseConfigurationFile(entities)) {
                writeDefaultConfigurationFile();
                parseConfigurationFile(entities);
        }

        parseArgs(argc, argv, &opt);
        SetGlobalDt(opt.dt);
		
        Logger(Info, " Recording spontaneous activity [%f sec @ %5.0f Hz ].\n", opt.tend, 1.0 / opt.dt);
        Logger(Debug, "Number of batches: %d.\n", opt.nBatches);
        Logger(Info, "Number of trials: %d.\n", opt.nTrials);
        Logger(Debug, "Inter-trial interval: %g sec.\n", (double) opt.iti * 1e-6);
        Logger(Debug, "Inter-batch interval: %g sec.\n", (double) opt.ibi * 1e-6);

        bool success;
        cnt = 1;
        total = opt.nBatches*opt.nTrials;
        for (i=0; i<opt.nBatches; i++) {
			for (k=0; k<opt.nTrials; k++, cnt++) {
				success = Simulate(entities, opt.tend);
				Logger(Important, " . ");
                if (!success || KILL_PROGRAM())
					goto endMain;
                if (k != opt.nTrials-1)
					usleep(opt.iti);
            }
			Logger(Important, "\n");
            if (i != opt.nBatches-1)
				usleep(opt.ibi);
        }
endMain:
        for (uint i=0; i<entities.size(); i++)
                delete entities[i];

        return retval;

#endif

}

