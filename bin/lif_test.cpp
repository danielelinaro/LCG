#include <iostream>
#include "utils.h"
#include "dynamical_entity.h"
#include "ou.h"
#include "synapses.h"
#include "neurons.h"

using namespace dynclamp;
using namespace dynclamp::synapses;
using namespace dynclamp::neurons;

int main()
{
        int i;
        double t, tend = 2, dt = GetGlobalDt();
        double taus[3] = {3e-3, 100e-3, 1000e-3};

        DynamicalEntity *entities[4];
        entities[0] = new OU(100, 10e-3, 100);
        entities[1] = new LIFNeuron(0.08, 0.0075, 0.0014, -65.2, -70, -50, 0);
        entities[2] = new TsodyksSynapse(0.0, 1.0, 0.03, taus);
        entities[3] = new LIFNeuron(0.08, 0.0075, 0.0014, -65.2, -70, -50, 0);

        //SetLoggingLevel(Debug);

        for (i=0; i<3; i++)
                entities[i]->connect(entities[i+1]);

        for (t=0.0; t<=tend; t+=dt) {
                ProcessEvents();
                for (i=0; i<4; i++)
                        entities[i]->readAndStoreInputs();
                std::cout << t;
                for (i=0; i<4; i++) {
                        std::cout << " " << entities[i]->output();
                        entities[i]->step();
                }
                std::cout << std::endl;
        }

        for (i=0; i<4; i++)
                delete entities[i];

        return 0;
}

