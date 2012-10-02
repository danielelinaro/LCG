#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "utils.h"
#include "poisson_generator.h"
#include "neurons.h"
#include "synapses.h"
#include "engine.h"
#include "connections.h"

using namespace dynclamp;
using namespace dynclamp::synapses;
using namespace dynclamp::generators;
using namespace dynclamp::neurons;

struct options {
        double rate;    // (Hz)
        int nEvents;    // number of events to generate
        ullong seed;    // seed for the random number generator
};

void PrintHelp(const char *msg = NULL) {
        if (msg)
                fprintf(stderr, "%s\n", msg);
        fprintf(stderr, "Usage: poisson_test -n <number of events> -r <rate> -h\n");
        exit(1);
}

void ParseArgs(int argc, char *argv[], struct options *opt)
{
        char c;
        int n;
        double f;

        // default values
        opt->rate = 1000;
        opt->nEvents = 1000;
        opt->seed = time(NULL);

        while((c = getopt(argc, argv, "n:r:s:h")) != -1) {
                switch(c) {
                case 'n':
                        n = atoi(optarg);
                        if (n > 0)
                                opt->nEvents = n;
                        else
                                PrintHelp("The number of events to simulate must be positive.");
                        break;
                case 'r':
                        f = atof(optarg);
                        if (f > 0)
                                opt->rate = f;
                        else
                                PrintHelp("The rate must be positive.");
                        break;
                case 's':
                        opt->seed = atoll(optarg);
                        break;
                case 'h':
                        PrintHelp();
                        break;
                default:
                        PrintHelp("Unknown option.");
                }
        }
}


int main(int argc, char *argv[])
{
        struct options opt;

        ParseArgs(argc, argv, &opt);

        int i;
        double t, tend = (double) opt.nEvents / opt.rate;
        double taus[2] = {0.1e-3,1e-3};
        Entity *entities[4];
        entities[0] = new Poisson(opt.rate, opt.seed);
        entities[1] = new SynapticConnection(1e-3, 1.);
        entities[2] = new Exp2Synapse(0.0, taus);
        entities[3] = new LIFNeuron(0.08, 0.0075, 0.0014, -65.2, -70, -50, 0);
        entities[0]->connect(entities[1]);
        entities[1]->connect(entities[2]);
        entities[2]->connect(entities[3]);
        Synapse *synapse = dynamic_cast<Synapse*>(entities[2]);

        for (i=0; i<4; i++)
                entities[i]->initialise();

        while ((t = GetGlobalTime()) <= tend) {
                ProcessEvents();
                for (i=0; i<4; i++)
                        entities[i]->readAndStoreInputs();
                fprintf(stdout, "%e %e\n", t, synapse->g());
                for (i=0; i<4; i++)
                        entities[i]->step();
                IncreaseGlobalTime();
        }

        for (i=0; i<4; i++)
                delete entities[i];

        return 0;
}

