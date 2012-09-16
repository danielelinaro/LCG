#include <stdio.h>
#include <time.h>
#include "utils.h"
#include "entity.h"
#include "ou.h"
#include "synapses.h"
#include "neurons.h"
#include "engine.h"
#include "connections.h"

using namespace dynclamp;
using namespace dynclamp::synapses;
using namespace dynclamp::neurons;

#define N_ENTITIES 7

int main()
{
        int i;
        double t, tend = 2;
        double taus[3] = {3e-3, 100e-3, 1000e-3};

        Entity *entities[N_ENTITIES];
        entities[0] = new OUcurrent(100, 100e-3, 100, time(NULL));
        entities[1] = new OUconductance(1, 2e-3, 0, 0, 7051983);
        entities[2] = new OUconductance(2, 10e-3, -80, 0, 5061983);
        entities[3] = new LIFNeuron(0.08, 0.0075, 0.0014, -65.2, -70, -50, 0);
        entities[4] = new Connection(3e-3);
        entities[5] = new TMGSynapse(0.0, 1.0, 0.03, taus);
        entities[6] = new LIFNeuron(0.08, 0.0075, 0.0014, -65.2, -70, -50, 0);

        //SetLoggingLevel(Debug);

        entities[0]->connect(entities[3]);
        entities[1]->connect(entities[3]);
        entities[2]->connect(entities[3]);
        entities[3]->connect(entities[4]);
        entities[4]->connect(entities[5]);
        entities[5]->connect(entities[6]);

        while ((t = GetGlobalTime()) <= tend) {
                ProcessEvents();
                for (i=0; i<N_ENTITIES; i++)
                        entities[i]->readAndStoreInputs();
                printf("%e", t);
                for (i=0; i<N_ENTITIES; i++) {
                        printf(" %13e", entities[i]->output());
                        entities[i]->step();
                }
                printf("\n");
                IncreaseGlobalTime();
        }

        for (i=0; i<N_ENTITIES; i++)
                delete entities[i];

        return 0;
}

