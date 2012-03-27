#include "entity.h"
#include "utils.h"
#include "stimulus_generator.h"

using namespace dynclamp;

int main(int argc, char *argv[])
{
        CommandLineOptions opt;
        double tend, dt;
        std::vector<Entity*> entities;
        dynclamp::generators::Stimulus *stimulus;

        if (!ParseCommandLineOptions(argc, argv, &opt)) {
                Logger(Critical, "Error while parsing command line arguments.\n"
                                 "Type \"hybrid_simulator -h\" for information on how to use this program.\n");
                exit(1);
        }
        SetGlobalDt(opt.dt);

        if (opt.configFile.compare("") == 0) {
                Logger(Critical, "No configuration file specified. Aborting...\n");
                exit(1);
        }

        if (!ParseConfigurationFile(opt.configFile, entities, &tend, &dt)) {
                Logger(Critical, "Error while parsing configuration file. Aborting...\n");
                exit(1);
        }

        Logger(Info, "opt.tend = %g sec.\n", opt.tend);
        if (opt.tend != -1)
                tend = opt.tend;
        if (tend == -1) {
                Logger(Critical, "The duration of the simulation was not specified. Aborting...\n");
                exit(1);
        }

        if (dt == -1) {
                dt = 1.0 / 20000;
                Logger(Info, "Using default timestep (%g sec -> %g Hz).\n", dt, 1.0/dt);
        }

        SetGlobalDt(dt);

        Logger(Info, "Number of trials: %d.\n", opt.nTrials);
        Logger(Info, "Inter-trial interval: %g sec.\n", (double) opt.iti * 1e-6);

        if (opt.stimulusFiles.size() > 0) {
                for (int i=0; i<entities.size(); i++) {
                        if ((stimulus = dynamic_cast<dynclamp::generators::Stimulus*>(entities[i])) != NULL)
                                break;
                }

                if (stimulus == NULL) {
                        Logger(Critical, "You need to have at least on Stimulus in your configuration file if you specify a stimulus file. Aborting.\n");
                        goto endMain;
                }

                Logger(Info, "Number of batches: %d.\n", opt.nBatches);
                Logger(Info, "Inter-batch interval: %g sec.\n", (double) opt.ibi * 1e-6);

                for (int i=0; i<opt.nBatches; i++) {
                        for (int j=0; j<opt.stimulusFiles.size(); j++) {
                                stimulus->setFilename(opt.stimulusFiles[j].c_str());
                                for (int k=0; k<opt.nTrials; k++) {
                                        ResetGlobalTime();
                                        Simulate(entities,tend);
                                        if (k != opt.nTrials-1)
                                                usleep(opt.iti);
                                }
                                if (j != opt.stimulusFiles.size()-1)
                                        usleep(opt.iti);
                        }
                        if (i != opt.nBatches-1)
                                usleep(opt.ibi);
                }

        }
        else {
                for (int i=0; i<opt.nTrials; i++) {
                        ResetGlobalTime();
                        Simulate(entities,tend);
                        if (i != opt.nTrials-1)
                                usleep(opt.iti);
                }
        }

endMain:
        for (int i=0; i<entities.size(); i++)
                delete entities[i];

        return 0;
}

