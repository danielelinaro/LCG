#include <stdlib.h>
#include <sys/types.h>

#include <vector>
#include <string>
#include <iostream>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>

#include "types.h"
#include "utils.h"
#include "entity.h"
#include "engine.h"
#include "stimulus_generator.h"
#include "frequency_clamp.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define FREQUENCY_CLAMP_VERSION 0.1
#define FREQUENCY_CLAMP_BANNER \
        "\n\tFrequency clamp\n" \
        "\nAuthor: Daniele Linaro (daniele@tnb.ua.ac.be)\n"

namespace po = boost::program_options;
namespace fs = boost::filesystem;

using boost::property_tree::ptree;
using namespace dynclamp;

struct options {
        double tend;
        useconds_t iti, ibi;
        uint nTrials, nBatches;
        std::string kernelFile;
        std::string frequency, baselineCurrent, tau, gp, gi, gd;
};

void parseArgs(int argc, char *argv[], options *opt);
bool parseConfigFile(const std::string& configfile, options *opt);
void runTrial(options *opt, const std::string kernelfile = "");

bool parseConfigFile(const std::string& configfile, options *opt)
{
        ptree pt;
        int i;
        try {
                read_xml(configfile, pt);
                opt->frequency = pt.get<std::string>("dynamicclamp.entities.frequencyclamp.frequency");
                opt->baselineCurrent = pt.get<std::string>("dynamicclamp.entities.frequencyclamp.baselineCurrent");
                opt->tau = pt.get<std::string>("dynamicclamp.entities.frequencyclamp.tau");
                opt->gp = pt.get<std::string>("dynamicclamp.entities.frequencyclamp.gp");
                opt->gi = pt.get<std::string>("dynamicclamp.entities.frequencyclamp.gi");
                opt->gd = pt.get<std::string>("dynamicclamp.entities.frequencyclamp.gd");
        } catch(std::exception e) {
                Logger(Critical, "Error while parsing configuration file: %s.\n", e.what());
                return false;
        }

        try {
                opt->tend = pt.get<double>("dynamicclamp.simulation.tend");
        } catch(std::exception e) {
                opt->tend = -1;
        }

        return true;
}

void parseArgs(int argc, char *argv[], options *opt)
{

        double tend, iti, ibi;
        uint nTrials, nBatches;
        std::string configfile, kernelfile;
        po::options_description description(
                        FREQUENCY_CLAMP_BANNER
                        "\nAllowed options");
        po::variables_map options;

        try {
                description.add_options()
                        ("help,h", "print help message")
                        ("version,v", "print version number")
                        ("config-file,c", po::value<std::string>(&configfile)->default_value("noisy_bg.xml"), "specify configuration file")
                        ("time,t", po::value<double>(&tend), "specify the duration of the simulation (in seconds)")
                        ("iti,i", po::value<double>(&iti)->default_value(0.25), "specify inter-trial interval (default: 0.25 sec)")
                        ("ibi,I", po::value<double>(&ibi)->default_value(0.25), "specify inter-batch interval (default: 0.25 sec)")
                        ("ntrials,n", po::value<uint>(&nTrials)->default_value(1), "specify the number of trials (how many times a stimulus is repeated)")
                        ("nbatches,N", po::value<uint>(&nBatches)->default_value(1), "specify the number of trials (how many times a batch of stimuli is repeated)");
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
                        std::cout << fs::path(argv[0]).filename() << " version " << FREQUENCY_CLAMP_VERSION << std::endl;
                        exit(0);
                }

                if (!fs::exists(configfile) || !parseConfigFile(configfile,opt)) {
                        std::cerr << "Configuration file \"" << configfile << "\" not found or invalid. Aborting...\n";
                        exit(1);
                }

                if (options.count("time")) {
                        opt->tend = tend;
                }
                else if (opt->tend == -1) {
                        std::cerr << "No simulation time specified. Aborting...\n";
                        exit(1);
                }

#ifdef HAVE_LIBCOMEDI
                if (!options.count("kernel-file") || !fs::exists(kernelfile)) {
                        std::cout << "Kernel file not specified or not found. Aborting...\n";
                        exit(1);
                }
                opt->kernelFile = kernelfile;
#endif

                opt->iti = (useconds_t) (iti * 1e6);
                opt->ibi = (useconds_t) (ibi * 1e6);
                opt->nTrials = nTrials;
                opt->nBatches = nBatches;

        } catch (std::exception e) {
                std::cout << e.what() << std::endl;
                exit(2);
        }

}

void runTrial(options *opt, const std::string kernelfile)
{
        std::vector<Entity*> entities(3);
        dictionary parameters;
        int i;

        try {
                // entity[0]
                parameters["compress"] = "false";
                entities[0] = EntityFactory("H5Recorder", parameters);
                
                // entity[1]
                parameters.clear();
#ifdef HAVE_LIBCOMEDI
                parameters["kernelFile"] = kernelfile;
                parameters["deviceFile"] = "/dev/comedi0";
                parameters["spikeThreshold"] = "-20";
                parameters["V0"] = "-57";
                parameters["inputSubdevice"] = "0";
                parameters["outputSubdevice"] = "1";
#ifdef AXON
                parameters["readChannel"] = "2";                // AI2
                parameters["writeChannel"] = "1";               // A01
                parameters["inputConversionFactor"] = "20";
                parameters["outputConversionFactor"] = "0.0025";
#else
                parameters["readChannel"] = "0";                // AI0
                parameters["writeChannel"] = "1";               // A01
                parameters["inputConversionFactor"] = "100";
                parameters["outputConversionFactor"] = "0.001";

#endif
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
                parameters["frequency"] = opt->frequency;
                parameters["baselineCurrent"] = opt->baselineCurrent;
                parameters["tau"] = opt->tau;
                parameters["gp"] = opt->gp;
                parameters["gi"] = opt->gi;
                parameters["gd"] = opt->gd;
                entities[2] = EntityFactory("FrequencyClamp", parameters);

                
                for (i=0; i<entities.size(); i++) {
                        if (entities[i] == NULL) {
                                Logger(Critical, "Entity #%d is NULL.\n", i);
                                exit(1);
                        }
                }

                // connect all the entities to the recorder
                for (i=1; i<entities.size(); i++)
                        entities[i]->connect(entities[0]);

                // connect the real neuron and the frequency clamp
                entities[1]->connect(entities[2]);
                entities[2]->connect(entities[1]);

        } catch (const char *msg) {
                Logger(Critical, "Error: %s.\n", msg);
                exit(1);
        }

        Simulate(entities, opt->tend);

        for (uint i=0; i<entities.size(); i++)
                delete entities[i];
}

int main(int argc, char *argv[])
{
        options opt;
        int i, j, k;

        SetLoggingLevel(Info);

        parseArgs(argc, argv, &opt);

        Logger(Info, FREQUENCY_CLAMP_BANNER);
        Logger(Info, "Number of batches: %d.\n", opt.nBatches);
        Logger(Info, "Number of trials: %d.\n", opt.nTrials);
        Logger(Info, "Inter-trial interval: %g sec.\n", (double) opt.iti * 1e-6);
        Logger(Info, "Inter-batch interval: %g sec.\n", (double) opt.ibi * 1e-6);

        for (i=0; i<opt.nBatches; i++) {
                for (j=0; j<opt.nTrials; j++) {
                        ResetGlobalTime();
#ifdef HAVE_LIBCOMEDI
                        runTrial(&opt, opt.kernelFile);
#else
                        runTrial(&opt);
#endif
                        if (j != opt.nTrials-1)
                                usleep(opt.iti);
                }
                if (i != opt.nBatches-1)
                        usleep(opt.ibi);
        }

        return 0;
}

