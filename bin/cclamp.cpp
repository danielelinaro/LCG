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

#define CCLAMP_VERSION 0.9
#define CC_BANNER \
        "\n\tCommand-line current clamp\n" \
        "\nAuthor: Daniele Linaro (daniele.linaro@ua.ac.be)\n" \
        "\nDeveloped at the Theoretical Neurobiology and Neuroengineering\n" \
        "Laboratory of the University of Antwerp.\n\n"

namespace po = boost::program_options;
namespace fs = boost::filesystem;
using boost::property_tree::ptree;
using namespace dynclamp;

enum cclamp_mode {DEFAULT, SPONTANEOUS, TRIGGERED_RECORDER};

struct CCoptions {
        useconds_t iti, ibi;
        uint nTrials, nBatches;
        double dt, tend, hold_value;
        std::vector<std::string> stimulusFiles;
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
        fprintf(fid, "[AnalogOutput0]\n");
        fprintf(fid, "device = /dev/comedi0\n");
        fprintf(fid, "range = [-10,+10]\n");
        fprintf(fid, "subdevice = 1\n");
        fprintf(fid, "channel = 1\n");
        fprintf(fid, "conversionFactor = 0.001\n");
        fprintf(fid, "reference = GRSE\n");
        fprintf(fid, "units = pA\n");

        fclose(fid);

        Logger(Info, "Successfully saved default configuration file in [%s].\n", configFile);
        return true;
}

void parseArgs(int argc, char *argv[], CCoptions *opt)
{
        double iti, ibi, freq, tend, hold_value;
        int verbosity;
        uint nTrials, nBatches;
        std::string stimfile, stimdir;
        std::string caption(CC_BANNER "\nAllowed options");
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
                        ("ibi,I", po::value<double>(&ibi)->default_value(0.25), "specify inter-batch interval (default: 0.25 sec)")
                        ("tend,s", po::value<double>(&tend)->default_value(-1.0), "duration of recording without stimulus (sec)")
                        ("ntrials,n", po::value<uint>(&nTrials)->default_value(1), "specify the number of trials (how many times a stimulus is repeated)")
                        ("nbatches,N", po::value<uint>(&nBatches)->default_value(1), "specify the number of trials (how many times a batch of stimuli is repeated)")
                        ("frequency,F", po::value<double>(&freq)->default_value(20000), "specify the sampling frequency")
                        ("hold-value,H", po::value<double>(&hold_value)->default_value(0), "specify the hold value (default 0 pA)")
                        ("stimfile,f", po::value<std::string>(&stimfile), "specify the stimulus file to use")
                        ("stimdir,d", po::value<std::string>(&stimdir), "specify the directory where stimulus files are located");

                po::store(po::parse_command_line(argc, argv, description), options);
                po::notify(options);    

                if (options.count("help")) {
                        std::cout << description << "\n";
                        exit(0);
                }

                if (options.count("version")) {
                        std::cout << fs::path(argv[0]).filename() << " version " << CCLAMP_VERSION << std::endl;
                        exit(0);
                }

                if (freq <= 0) {
                        std::cerr << "The sampling frequency must be positive.\n";
                        exit(1);
                }

                if (options.count("stimfile")) {
                        if (!fs::exists(stimfile)) {
                                std::cout << "Stimulus file \"" << stimfile << "\" not found. Aborting...\n";
                                exit(1);
                        }
                        opt->stimulusFiles.push_back(stimfile);
                }
                else if (options.count("stimdir")) {
                        if (!fs::exists(stimdir)) {
                                std::cout << "Directory \"" << stimdir << "\" not found. Aborting...\n";
                                exit(1);
                        }
                        for (fs::directory_iterator it(stimdir); it != fs::directory_iterator(); it++) {
                                if (! fs::is_directory(it->status())) {
                                        opt->stimulusFiles.push_back(it->path().string());
                                }
                        }
                }
				else if (options.count("tend")) {
					std::cout << "No stimulus detected only recording inputs.\n";
				}
                else {
                        std::cout << description << "\n";
                        std::cout << "ERROR: you have to specify one of [stimulus file] or [stimulus directory]. Aborting...\n";
                        exit(1);
                }

                if (verbosity < All || verbosity > Critical) {
                        Logger(Important, "The verbosity level must be between %d and %d.\n", All, Critical);
                        exit(1);
                }

                SetLoggingLevel(static_cast<LogLevel>(verbosity));

                opt->dt = 1.0 / freq;
                opt->iti = (useconds_t) (iti * 1e6);
                opt->ibi = (useconds_t) (ibi * 1e6);
                opt->nTrials = nTrials;
                opt->nBatches = nBatches;
				opt->tend = tend;
				opt->hold_value = hold_value;

        } catch (std::exception e) {
                std::cout << e.what() << std::endl;
                exit(2);
        }

}

bool parseConfigurationFile(std::vector<Entity*>& entities, double hold_value = 0.0, enum cclamp_mode mode = DEFAULT)
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
                
				switch(mode) 
				{
					case SPONTANEOUS:
						Logger(Debug,"Ignoring Waveform.\n");
						break;
					default:
						parameters.clear();
						parameters["id"] = "1";
						parameters["units"] = "pA";
						entities.push_back( EntityFactory("Waveform", parameters) );
						break;
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
				
				switch(mode) 
				{
					case SPONTANEOUS:
						Logger(Debug, "Connecting the analog inputs [%d to %d] to the recorder.\n",
									entities.size()-AI_cnt, entities.size());
						for (int i=entities.size()-AI_cnt; i<entities.size(); i++)
							entities[i]->connect(entities[0]);
						break;
					default:
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

						Logger(Info,"Using %f as a holding value.\n",hold_value);
						if(hold_value != 0.0)
						{
							std::stringstream hold_value_str;
							hold_value_str << hold_value;
							std::stringstream id;
							id << entities.back()->id() + 1;
							parameters.clear();
							parameters["id"] = id.str();
							parameters["units"] = "pA";
							parameters["value"] = hold_value_str.str();
							entities.push_back( EntityFactory("Constant", parameters) );
							Logger(Debug, "Connecting the constant to the [%d to %d] to the recorder.\n",
									entities.size(), 0);
							entities.back()->connect(entities[0]);
							Logger(Debug, "Connecting the constant to the analog output.\n");
							entities.back()->connect(entities[entities.size()-2]);
						}   
						break;
					
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
                Logger(Critical, "Unable to setup signal catching functionalities. Aborting.\n");
                exit(1);
        }

        std::vector<Entity*> entities;
        dynclamp::generators::Waveform *stimulus;
        CCoptions opt;
        int i, j, k, cnt, total, retval = 0;
		enum cclamp_mode mode = DEFAULT;
        SetLoggingLevel(Info);
        
		parseArgs(argc, argv, &opt);
		if (opt.tend > 0.0) 
		{
                Logger(Important, "Recording in spontaneous mode.\n");
				mode = SPONTANEOUS;
		}
        if (! parseConfigurationFile(entities, opt.hold_value, mode)) {
                writeDefaultConfigurationFile();
                parseConfigurationFile(entities, opt.hold_value, mode);
        }
		switch(mode)
		{
			case SPONTANEOUS:
				break;	
			default:
		        for (i=0; i<entities.size(); i++) {
			        if ((stimulus = dynamic_cast<dynclamp::generators::Waveform*>(entities[i])) != NULL)
			            break;
				}
				if (i == entities.size()) {
					Logger(Critical, "No stimulus present.\n");
					retval = 1;
					goto endMain;
				}
				break;
		}

        SetGlobalDt(opt.dt);

        Logger(Info, CC_BANNER);
        Logger(Info, "Number of batches: %d.\n", opt.nBatches);
        Logger(Info, "Number of trials: %d.\n", opt.nTrials);
        Logger(Info, "Inter-trial interval: %g sec.\n", (double) opt.iti * 2e-6);
        Logger(Info, "Holding current: %g pA.\n", opt.hold_value);
        Logger(Info, "Inter-batch interval: %g sec.\n", (double) opt.ibi * 1e-6);

        bool success;
        cnt = 1;
		switch(mode)
		{
			case SPONTANEOUS:
			    total = opt.nBatches*opt.nTrials;
				for	(i=0; i<opt.nBatches; i++) {
					    for (k=0; k<opt.nTrials; k++, cnt++) {
                                Logger(Important, "Trial: %d of %d.\n", cnt, total);
                                success = Simulate(entities, opt.tend);
                                if (!success || KILL_PROGRAM())
                                        goto endMain;
                                if (k != opt.nTrials-1)
                                        usleep(opt.iti);
                        }
					if (i != opt.nBatches-1)
                        usleep(opt.ibi);
				}
				break;
			default:
			    total = opt.nBatches*opt.stimulusFiles.size()*opt.nTrials;
				for	(i=0; i<opt.nBatches; i++) {
					for (j=0; j<opt.stimulusFiles.size(); j++) {
                        stimulus->setStimulusFile(opt.stimulusFiles[j].c_str());
					    for (k=0; k<opt.nTrials; k++, cnt++) {
                                Logger(Important, "Trial: %d of %d.\n", cnt, total);
                                success = Simulate(entities, stimulus->duration());
                                if (!success || KILL_PROGRAM())
                                        goto endMain;
                                if (k != opt.nTrials-1)
                                        usleep(opt.iti);
                        }
                        if (j != opt.stimulusFiles.size()-1)
                                usleep(opt.iti);
					}
					if (i != opt.nBatches-1)
                        usleep(opt.ibi);
				}
				break;
		}

endMain:
        for (uint i=0; i<entities.size(); i++)
                delete entities[i];

        return retval;

#endif

}

