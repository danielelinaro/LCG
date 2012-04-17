#include <stdlib.h>
#include <sys/types.h>

#include <vector>
#include <string>
#include <iostream>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include "types.h"
#include "utils.h"
#include "entity.h"
#include "engine.h"
#include "stimulus_generator.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define HEKA
//#define AXON

#if defined(HEKA) && defined(AXON)
#error Only one of HEKA or AXON should be defined.
#endif

#define INSUBDEV "0"
#define OUTSUBDEV "1"
#define WRITECHAN "1"
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
 
#define AFS_CLONE_VERSION 0.1
#define AFS_BANNER \
        "\n\tArbitrary Function Stimulator (AFS)\n" \
        "\nAuthor: Daniele Linaro (daniele@tnb.ua.ac.be)\n" \
        "\nThis program has some of the basic functionalities provided by the AFS developed\n" \
        "by Mike Wijnants at the Theoretical Neurobiology lab in Antwerp.\n"

namespace po = boost::program_options;
namespace fs = boost::filesystem;

using namespace dynclamp;

struct AFSoptions {
        useconds_t iti, ibi;
        uint nTrials, nBatches;
        double dt;
        std::vector<std::string> stimulusFiles;
};

void parseArgs(int argc, char *argv[], AFSoptions *opt)
{
        double iti, ibi, freq;
        uint nTrials, nBatches;
        std::string stimfile, stimdir;
        std::string caption(AFS_BANNER "\nAllowed options");
        po::options_description description(caption);
        po::variables_map options;

        try {
                description.add_options()
                        ("help,h", "print help message")
                        ("version,v", "print version number")
                        ("iti,i", po::value<double>(&iti)->default_value(0.25), "specify inter-trial interval (default: 0.25 sec)")
                        ("ibi,I", po::value<double>(&ibi)->default_value(0.25), "specify inter-batch interval (default: 0.25 sec)")
                        ("ntrials,n", po::value<uint>(&nTrials)->default_value(1), "specify the number of trials (how many times a stimulus is repeated)")
                        ("nbatches,N", po::value<uint>(&nBatches)->default_value(1), "specify the number of trials (how many times a batch of stimuli is repeated)")
                        ("frequency,F", po::value<double>(&freq)->default_value(20000), "specify the sampling frequency")
                        ("stimfile,f", po::value<std::string>(&stimfile), "specify the stimulus file to use")
                        ("stimdir,d", po::value<std::string>(&stimdir), "specify the directory where stimulus files are located");

                po::store(po::parse_command_line(argc, argv, description), options);
                po::notify(options);    

                if (options.count("help")) {
                        std::cout << description << "\n";
                        exit(0);
                }

                if (options.count("version")) {
                        std::cout << fs::path(argv[0]).filename() << " version " << AFS_CLONE_VERSION << std::endl;
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

void runStimulus(const std::string& stimfile)
{
        Logger(Info, "Processing stimulus file [%s].\n", stimfile.c_str());

#ifdef HAVE_LIBCOMEDI

        std::vector<Entity*> entities;
        dictionary parameters;
        double tend;

        try {
                parameters["id"] = "0";
                parameters["compress"] = "false";
                entities.push_back( EntityFactory("H5Recorder", parameters) );
                
                parameters.clear();
                parameters["id"] = "1";
                parameters["deviceFile"] = "/dev/comedi0";
                parameters["range"] = "[-10,+10]";
                parameters["inputSubdevice"] = INSUBDEV;
                parameters["readChannel"] = READCHAN;
                parameters["inputConversionFactor"] = INFACT;
                parameters["reference"] = REF;
                entities.push_back( EntityFactory("AnalogInput", parameters) );

                parameters.clear();
                parameters["id"] = "2";
                parameters["deviceFile"] = "/dev/comedi0";
                parameters["outputSubdevice"] = OUTSUBDEV;
                parameters["writeChannel"] = WRITECHAN;
                parameters["outputConversionFactor"] = OUTFACT;
                parameters["reference"] = REF;
                entities.push_back( EntityFactory("AnalogOutput", parameters) );

                parameters.clear();
                parameters["id"] = "3";
                parameters["filename"] = stimfile;
                entities.push_back( EntityFactory("Waveform", parameters) );

                Logger(Debug, "Connecting the analog input to the recorder.\n");
                entities[1]->connect(entities[0]);
                //Logger(Debug, "Connecting the analog output to the recorder.\n");
                //entities[2]->connect(entities[0]);
                Logger(Debug, "Connecting the stimulus to the recorder.\n");
                entities[3]->connect(entities[0]);
                Logger(Debug, "Connecting the stimulus to the analog output.\n");
                entities[3]->connect(entities[2]);

                tend = dynamic_cast<generators::Waveform*>(entities[3])->duration();

        } catch (const char *msg) {
                Logger(Critical, "Error: %s\n", msg);
                exit(1);
        }

        Simulate(entities, tend);

        for (uint i=0; i<entities.size(); i++)
                delete entities[i];
#endif
}

int main(int argc, char *argv[])
{
        AFSoptions opt;
        int i, j, k;

        SetLoggingLevel(Info);
        parseArgs(argc, argv, &opt);
        SetGlobalDt(opt.dt);

        Logger(Info, AFS_BANNER);
        Logger(Info, "Number of batches: %d.\n", opt.nBatches);
        Logger(Info, "Number of trials: %d.\n", opt.nTrials);
        Logger(Info, "Inter-trial interval: %g sec.\n", (double) opt.iti * 1e-6);
        Logger(Info, "Inter-batch interval: %g sec.\n", (double) opt.ibi * 1e-6);

        for (i=0; i<opt.nBatches; i++) {
                for (j=0; j<opt.stimulusFiles.size(); j++) {
                        for (k=0; k<opt.nTrials; k++) {
                                ResetGlobalTime();
                                runStimulus(opt.stimulusFiles[j]);
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

