#include <cstdio>>
#include "utils.h"
#include "recorders.h"
#include "neurons.h"
#include "synapses.h"

using namespace dynclamp;
using namespace dynclamp::recorders;
using namespace dynclamp::synapses;
using namespace dynclamp::neurons;

int main()
{
        uint i;
        double t;
        double tend = 2;
        double taus[3] = {3e-3, 100e-3, 1000e-3};
        bool compress = false;
        Entity *entities[3];
        entities[0] = new H5Recorder("autapse.h5", compress);
        entities[1] = new LIFNeuron(0.08, 0.0075, 0.0014, -65.2, -70, -50, 220);
        entities[2] = new TMGSynapse(-80.0, 20.0, 0.03, taus);
        for (i=1; i<3; i++) {
                entities[i]->connect(entities[0]);
        }
        entities[1]->connect(entities[2]);
        entities[2]->connect(entities[1]);

        try {
                while((t=GetGlobalTime()) <= tend) {
                        ProcessEvents();
                        for (i=0; i<3; i++)
                                entities[i]->readAndStoreInputs();
                        IncreaseGlobalTime();
                        for (i=0; i<3; i++)
                                entities[i]->step();
                }
        } catch (const char *msg) {
                fprintf(stderr, "ERROR: %s\n", msg);
        }

        for (i=0; i<3; i++)
                delete entities[i];

        return 0;
}

