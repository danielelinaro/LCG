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

#define CONFIG_FILE ".cclamprc"

namespace po = boost::program_options;
namespace fs = boost::filesystem;
using boost::property_tree::ptree;
using namespace lcg;

typedef enum {DEFAULT, SPONTANEOUS, TRIGGERED} recording_mode;

struct options {
        options() : tend(0), dt(0), iti(0), ibi(0),
                nTrials(0), nBatches(0), holdValue(0),
                stimulusFiles(), mode(DEFAULT) {}
        double tend, dt;
        useconds_t iti, ibi;
        uint nTrials, nBatches;
        double holdValue;
        std::vector<std::string> stimulusFiles;
        recording_mode mode;
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
        return true;
}

void parseArgs(int argc, char *argv[], options *opts)
{
        double iti, ibi, holdValue, samplingRate, tend;
        int verbosity;
        uint nTrials, nBatches;
        std::string stimfile, stimdir;
        std::string caption("\nUsage: cclamp [options]\n\nAllowed options");
        po::options_description description(caption);
        po::variables_map options;

        try {
                char msg[100];
                sprintf(msg, "select verbosity level (%d for maximum, %d for minimum verbosity)", All, Critical);
                description.add_options()
                        ("help,h", "print help message")
                        ("version,v", "print version number")
                        ("verbosity,V", po::value<int>(&verbosity)->default_value(Info), msg)
                        ("frequency,F", po::value<double>(&samplingRate)->default_value(20000.), "sampling rate")
                        ("iti,i", po::value<double>(&iti), "inter-trial interval")
                        ("ibi,I", po::value<double>(&ibi)->default_value(0),
                         "inter-batch interval (default: same as inter-trial interval)")
                        ("ntrials,n", po::value<uint>(&nTrials)->default_value(1),
                         "number of trials (how many times a stimulus is repeated)")
                        ("nbatches,N", po::value<uint>(&nBatches)->default_value(1),
                         "number of batches (how many times a batch of stimuli is repeated)")
                        ("hold-value,H", po::value<double>(&holdValue)->default_value(0), "holding value")
                        ("duration,d", po::value<double>(&tend), "duration of the recording")
                        ("stimfile,f", po::value<std::string>(&stimfile), "stimulus file")
                        ("stimdir,D", po::value<std::string>(&stimdir), "directory containing the stimulus files");

                po::store(po::parse_command_line(argc, argv, description), options);
                po::notify(options);    

                if (options.count("help")) {
                        std::cout << description << "\n";
                        exit(0);
                }

                if (options.count("version")) {
                        Logger(Info, "%s version %s.\n", fs::path(argv[0]).filename().c_str(), "1");
                        exit(0);
                }

                if (options.count("stimfile")) {
                        if (!fs::exists(stimfile)) {
                                Logger(Critical, "%s: no such file.\n", stimfile.c_str());
                                exit(1);
                        }
                        opts->stimulusFiles.push_back(stimfile);
                }
                else if (options.count("stimdir")) {
                        if (!fs::exists(stimdir)) {
                                Logger(Critical, "%s: no such directory.\n", stimdir.c_str());
                                exit(1);
                        }
                        for (fs::directory_iterator it(stimdir); it != fs::directory_iterator(); it++) {
                                if (! fs::is_directory(it->status())) {
                                        opts->stimulusFiles.push_back(it->path().string());
                                }
                        }
                        if (opts->stimulusFiles.size() == 0) {
                                Logger(Critical, "%s: empty directory.\n", stimdir.c_str());
                                exit(1);
                        }
                }
		else if (options.count("duration")) {
                        opts->tend = tend;
                        opts->mode = SPONTANEOUS;
		        Logger(Debug, "No stimulus specified, recording only.\n");
                        if (holdValue != 0.)
                                Logger(Important, "Ignoring the holding value.\n");
		}
                else {
                        Logger(Critical, "You must specify either a stimulus file (-f switch) or "
                                         "a directory containing stimulus files (-D switch). In "
                                         "alternative, you can use the -d switch to simply record.\n");
                        exit(1);
                }

                if (verbosity < All || verbosity > Critical) {
                        Logger(Important, "The verbosity level must be between %d and %d.\n", All, Critical);
                        exit(1);
                }

                SetLoggingLevel(static_cast<LogLevel>(verbosity));

                if (samplingRate <= 0) {
                        Logger(Critical, "The sampling rate must be positive.\n");
                        exit(1);
                }

                if (nTrials > 1 && !options.count("iti")) {
                        Logger(Critical, "You must specify the interval between repetitions (-i switch).\n");
                        exit(1);
                }
                else {
                        opts->iti = (useconds_t) (iti * 1e6);
                        opts->ibi = (useconds_t) (iti * 1e6);
                }

                if (options.count("ibi"))
                        opts->ibi = (useconds_t) (ibi * 1e6);

                opts->dt = 1.0 / samplingRate;
                opts->nTrials = nTrials;
                opts->nBatches = nBatches;
		opts->holdValue = holdValue;

        } catch (std::exception e) {
                std::cout << e.what() << std::endl;
                exit(2);
        }
}

bool parseConfigurationFile(std::vector<Entity*>& entities, const options *opts)
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
                Logger(Critical, "Unable to setup signal catching functionalities.\n");
                exit(1);
        }

        std::vector<Entity*> entities;
        lcg::generators::Waveform *stimulus;
        options opts;
        int i, j, k, cnt, total, retval = 0;

        SetLoggingLevel(Info);
        
	parseArgs(argc, argv, &opts);

        if (! parseConfigurationFile(entities, &opts)) {
                writeDefaultConfigurationFile();
                parseConfigurationFile(entities, &opts);
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

