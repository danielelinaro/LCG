#include <stdlib.h>
#include <sys/types.h>

#include <vector>
#include <string>
#include <iostream>
#include <sstream>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>

#include "types.h"
#include "utils.h"
#include "entity.h"
#include "engine.h"
#include "waveform.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define HEKA
//#define AXON

#if defined(HEKA) && defined(AXON)
#error Only one of HEKA or AXON should be defined.
#endif

#define INSUBDEV  "0"
#define OUTSUBDEV "1"
#define WRITECHAN "1"
#define DELAY     "2"
#if defined(HEKA)
// HEKA specific parameters - start
#define READCHAN "0"
#define INFACT   "100"
#define OUTFACT  "0.001"
#define REF      "GRSE"
#elif defined(AXON)
// AXON specific parameters - start
#define READCHAN "2"
#define INFACT   "20"
#define OUTFACT  "0.0025"
#define REF      "NRSE"
#else
#error One of HEKA or AXON should be defined.
#endif

#define NOISY_BG_CLONE_VERSION 0.1
#define NOISY_BG_BANNER \
        "\n\tArbitrary Function Stimulator with noisy background\n" \
        "\nAuthor: Daniele Linaro (daniele@tnb.ua.ac.be)\n"

namespace po = boost::program_options;
namespace fs = boost::filesystem;

using boost::property_tree::ptree;
using namespace dynclamp;

struct OUoptions {
        std::string sigma, tau, E, G0, interval, seed;
};

struct options {
        useconds_t iti, ibi;
        uint nTrials, nBatches;
        double dt;
        std::vector<std::string> stimulusFiles;
        std::string kernelFile;
        OUoptions ou[2];
};

void parseArgs(int argc, char *argv[], options *opt);
bool parseConfigFile(const std::string& configfile, options *opt);
void runStimulus(OUoptions *opt, const std::string& stimfile, const std::string kernelfile = "");

bool parseConfigFile(const std::string& configfile, options *opt)
{
        ptree pt;
        int i;
        try {
                read_xml(configfile, pt);
                i = 0;
                BOOST_FOREACH(ptree::value_type &v,
                              pt.get_child("dynamicclamp.entities")) {
                        opt->ou[i].sigma = v.second.get<std::string>("sigma");
                        opt->ou[i].tau = v.second.get<std::string>("tau");
                        opt->ou[i].E = v.second.get<std::string>("E");
                        opt->ou[i].G0 = v.second.get<std::string>("G0");
                        try {
                                opt->ou[i].interval = v.second.get<std::string>("interval");
                        } catch(...) {
                                opt->ou[i].interval = "0,86400";
                        }
                        try {
                                opt->ou[i].seed = v.second.get<std::string>("seed");
                        } catch(...) {
                                opt->ou[i].seed = "-1";
                        }
                        if (++i == 2)
                                break;
                }
        } catch(std::exception e) {
                Logger(Critical, "Error while parsing configuration file: %s.\n", e.what());
                return false;
        }
        return true;
}

void parseArgs(int argc, char *argv[], options *opt)
{

        double iti, ibi, freq;
        uint nTrials, nBatches;
        std::string stimfile, stimdir, configfile, kernelfile;
        po::options_description description(
                        NOISY_BG_BANNER
                        "\nAllowed options");
        po::variables_map options;

        try {
                description.add_options()
                        ("help,h", "print help message")
                        ("version,v", "print version number")
                        ("config-file,c", po::value<std::string>(&configfile)->default_value("noisy_bg.xml"), "specify configuration file")
                        ("iti,i", po::value<double>(&iti)->default_value(0.25), "specify inter-trial interval (default: 0.25 sec)")
                        ("ibi,I", po::value<double>(&ibi)->default_value(0.25), "specify inter-batch interval (default: 0.25 sec)")
                        ("ntrials,n", po::value<uint>(&nTrials)->default_value(1), "specify the number of trials (how many times a stimulus is repeated)")
                        ("nbatches,N", po::value<uint>(&nBatches)->default_value(1), "specify the number of trials (how many times a batch of stimuli is repeated)")
                        ("frequency,F", po::value<double>(&freq)->default_value(20000), "specify the sampling frequency")
                        ("stim-file,f", po::value<std::string>(&stimfile), "specify the stimulus file to use")
                        ("stim-dir,d", po::value<std::string>(&stimdir), "specify the directory where stimulus files are located");
#ifdef HAVE_LIBCOMEDI
                description.add_options()("kernel-file,k", po::value<std::string>(&kernelfile), "specify kernel file");
#endif

                po::store(po::parse_command_line(argc, argv, description), options);
                po::notify(options);    

                if (options.count("help")) {
                        std::cout << description << "\n";
                        exit(0);
                }

                if (options.count("version")) {
                        std::cout << fs::path(argv[0]).filename() << " version " << NOISY_BG_CLONE_VERSION << std::endl;
                        exit(0);
                }

                if (freq <= 0) {
                        std::cerr << "The sampling frequency must be positive.\n";
                        exit(1);
                }

                if (!fs::exists(configfile) || !parseConfigFile(configfile,opt)) {
                        std::cout << "Configuration file \"" << configfile << "\" not found or invalid. Aborting...\n";
                        exit(1);
                }

#ifdef HAVE_LIBCOMEDI
                if (!options.count("kernel-file") || !fs::exists(kernelfile)) {
                        std::cout << "Kernel file not specified or not found. Aborting...\n";
                        exit(1);
                }
                opt->kernelFile = kernelfile;
#endif

                if (options.count("stim-file")) {
                        if (!fs::exists(stimfile)) {
                                std::cout << "Stimulus file \"" << stimfile << "\" not found. Aborting...\n";
                                exit(1);
                        }
                        opt->stimulusFiles.push_back(stimfile);
                }
                else if (options.count("stim-dir")) {
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
                else {
                        std::cout << description << "\n";
                        std::cout << "ERROR: you have to specify one of [stimulus file] or [stimulus directory]. Aborting...\n";
                        exit(1);
                }

                opt->dt = 1.0 / freq;
                opt->iti = (useconds_t) (iti * 1e6);
                opt->ibi = (useconds_t) (ibi * 1e6);
                opt->nTrials = nTrials;
                opt->nBatches = nBatches;

        } catch (std::exception e) {
                std::cout << e.what() << std::endl;
                exit(2);
        }

}

void runStimulus(OUoptions *opt, const std::string& stimfile, const std::string kernelfile)
{
        Logger(Info, "Processing stimulus file [%s].\n", stimfile.c_str());

        std::vector<Entity*> entities(5);
        string_dictypedef parameters;
        double tend;
        int i;

        try {
                // entity[0]
                parameters["id"] = "0";
                parameters["compress"] = "false";
                entities[0] = EntityFactory("H5Recorder", parameters);
                
                // entity[1]
                parameters.clear();
                parameters["id"] = "1";
#ifdef HAVE_LIBCOMEDI
                parameters["kernelFile"] = kernelfile;
                parameters["deviceFile"] = "/dev/comedi0";
                parameters["inputSubdevice"] = INSUBDEV;
                parameters["outputSubdevice"] = OUTSUBDEV;
                parameters["readChannel"] = READCHAN;
                parameters["writeChannel"] = WRITECHAN;
                parameters["inputConversionFactor"] = INFACT;
                parameters["outputConversionFactor"] = OUTFACT;
                parameters["reference"] = REF;
                parameters["delay"] = DELAY;
                parameters["spikeThreshold"] = "-20";
                parameters["V0"] = "-57";
                entities[1] = EntityFactory("RealNeuron", parameters);
#else
                parameters["C"] = "0.08";
                parameters["tau"] = "0.0075";
                parameters["tarp"] = "0.0014";
                parameters["Er"] = "-65.2";
                parameters["E0"] = "-70";
                parameters["Vth"] = "-50";
                parameters["Iext"] = "0";
                entities[1] = EntityFactory("LIFNeuron", parameters);
#endif

                // entity[2]
                parameters.clear();
                parameters["id"] = "2";
                parameters["filename"] = stimfile;
                entities[2] = EntityFactory("Waveform", parameters);

                // entity[3] & entity[4]
                for (i=0; i<2; i++) {
                        std::stringstream ss;
                        ss << 3+i;
                        parameters.clear();
                        parameters["id"] = ss.str();
                        parameters["sigma"] = opt[i].sigma;
                        parameters["tau"] = opt[i].tau;
                        parameters["E"] = opt[i].E;
                        parameters["G0"] = opt[i].G0;
                        parameters["interval"] = opt[i].interval;
                        if (opt[i].seed.compare("-1") != 0) {
                                parameters["seed"] = opt[i].seed;
                        }
                        else {
                                std::stringstream ss;
                                ss << GetRandomSeed();
                                parameters["seed"] = ss.str();
                        }
                        entities[3+i] = EntityFactory("OUconductance", parameters);
                }
                
                for (i=0; i<entities.size(); i++) {
                        if (entities[i] == NULL) {
                                Logger(Critical, "Entity #%d is NULL.\n", i);
                                exit(1);
                        }
                }

                // connect all the entities to the recorder
                for (i=1; i<entities.size(); i++)
                        entities[i]->connect(entities[0]);

                // connect the stimuli to the neuron
                for (i=2; i<entities.size(); i++)
                        entities[i]->connect(entities[1]);

                tend = dynamic_cast<generators::Waveform*>(entities[2])->duration();

        } catch (const char *msg) {
                Logger(Critical, "Error: %s.\n", msg);
                exit(1);
        }

        Simulate(entities, tend);

        for (uint i=0; i<entities.size(); i++)
                delete entities[i];
}

int main(int argc, char *argv[])
{
        options opt;
        int i, j, k;

        SetLoggingLevel(Info);

        parseArgs(argc, argv, &opt);
        SetGlobalDt(opt.dt);

        Logger(Info, NOISY_BG_BANNER);
        Logger(Info, "Number of batches: %d.\n", opt.nBatches);
        Logger(Info, "Number of trials: %d.\n", opt.nTrials);
        Logger(Info, "Inter-trial interval: %g sec.\n", (double) opt.iti * 1e-6);
        Logger(Info, "Inter-batch interval: %g sec.\n", (double) opt.ibi * 1e-6);

        for (i=0; i<opt.nBatches; i++) {
                for (j=0; j<opt.stimulusFiles.size(); j++) {
                        for (k=0; k<opt.nTrials; k++) {
                                ResetGlobalTime();
#ifdef HAVE_LIBCOMEDI
                                runStimulus(opt.ou, opt.stimulusFiles[j], opt.kernelFile);
#else
                                runStimulus(opt.ou, opt.stimulusFiles[j]);
#endif
                                if (k != opt.nTrials-1)
                                        usleep(opt.iti);
                        }
                        if (j != opt.stimulusFiles.size()-1)
                                usleep(opt.iti);
                }
                if (i != opt.nBatches-1)
                        usleep(opt.ibi);
        }

        return 0;
}

