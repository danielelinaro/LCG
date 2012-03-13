#include "entity.h"
#include "utils.h"

using namespace dynclamp;

int main(int argc, char *argv[])
{
        CommandLineOptions opt;
        double tend, dt;
        std::vector<Entity*> entities;

        if (!ParseCommandLineOptions(argc, argv, &opt)) {
                Logger(Critical, "Error while parsing command line arguments.\n"
                                 "Type \"hybrid_simulator -h\" for information on how to use this program.\n");
                exit(1);
        }

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

        Logger(Info, "Number of batches: %d.\n", opt.nBatches);
        Logger(Info, "Number of trials: %d.\n", opt.nTrials);
        Logger(Info, "Inter-trial interval: %g sec.\n", (double) opt.iti * 1e-6);
        Logger(Info, "Inter-batch interval: %g sec.\n", (double) opt.ibi * 1e-6);

        for (int i=0; i<opt.nBatches; i++) {
                for (int j=0; j<opt.nTrials; j++) {
                        ResetGlobalTime();
                        Simulate(entities,tend);
                        if (j != opt.nTrials-1)
                                usleep(opt.iti);
                }
                if (i != opt.nBatches-1)
                        usleep(opt.ibi);
        }

        for (int i=0; i<entities.size(); i++)
                delete entities[i];

        return 0;
}

