#include <stdlib.h>
#include "entity.h"
#include "utils.h"
#include "waveform.h"
#include "engine.h"

using namespace dynclamp;

int main(int argc, char *argv[])
{
        CommandLineOptions opt;

        if (!SetupSignalCatching()) {
                Logger(Critical, "Unable to setup signal catching functionalities. Aborting.\n");
                exit(1);
        }

        ParseCommandLineOptions(argc, argv, &opt);

        if (opt.configFile.compare("") == 0) {
                Logger(Critical, "No configuration file specified. Aborting.\n");
                exit(1);
        }

        double tend, dt;
        std::vector<Entity*> entities;
        dynclamp::generators::Waveform *stimulus;

        if (!ParseConfigurationFile(opt.configFile, entities, &tend, &dt)) {
                Logger(Critical, "Error while parsing configuration file. Aborting.\n");
                exit(1);
        }

        Logger(Debug, "opt.tend = %g sec.\n", opt.tend);
        if (opt.tend != -1)
                // the duration specified in the command line has precedence over the one in the configuration file
                tend = opt.tend;

        if (opt.dt != -1)
                // the timestep specified in the command line has precedence over the one in the configuration file
                dt = opt.dt;
        if (dt == -1) {
                dt = 1.0 / 20000;
                Logger(Info, "Using default timestep (%g sec -> %g Hz).\n", dt, 1.0/dt);
        }

        SetGlobalDt(dt);

        Logger(Info, "Number of trials: %d.\n", opt.nTrials);
        Logger(Info, "Inter-trial interval: %g sec.\n", (double) opt.iti * 1e-6);

        if (opt.stimulusFiles.size() > 0) {
                for (int i=0; i<entities.size(); i++) {
                        if ((stimulus = dynamic_cast<dynclamp::generators::Waveform*>(entities[i])) != NULL)
                                break;
                }

                if (stimulus == NULL) {
                        Logger(Critical, "You need to have at least one Stimulus in your configuration file if you specify a stimulus file. Aborting.\n");
                        goto endMain;
                }

                Logger(Info, "Number of batches: %d.\n", opt.nBatches);
                Logger(Info, "Inter-batch interval: %g sec.\n", (double) opt.ibi * 1e-6);

                for (int i=0; i<opt.nBatches; i++) {
                        for (int j=0; j<opt.stimulusFiles.size(); j++) {
                                stimulus->setStimulusFile(opt.stimulusFiles[j].c_str());
                                for (int k=0; k<opt.nTrials; k++) {
                                        Logger(Info, "Batch: %d, stimulus: %d, trial: %d. (of %d,%d,%d).\n", i+1, j+1, k+1, opt.nBatches, opt.stimulusFiles.size(), opt.nTrials);
                                        ResetGlobalTime();
                                        Simulate(entities,stimulus->duration());
                                        if (TERMINATE())
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

        }
        else {
                if (tend == -1) {
                        Logger(Critical, "The duration of the simulation was not specified. Aborting.\n");
                        exit(1);
                }
                for (int i=0; i<opt.nTrials; i++) {
                        Logger(Important, "Trial: %d of %d.\n", i+1,opt.nTrials);
                        ResetGlobalTime();
                        Simulate(entities,tend);
                        if (TERMINATE())
                                goto endMain;
                        if (i != opt.nTrials-1)
                                usleep(opt.iti);
                }
        }

endMain:
        for (int i=0; i<entities.size(); i++)
                delete entities[i];

        return 0;
}

