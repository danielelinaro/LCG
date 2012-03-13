#include <stdio.h>
#include <vector>
#include "utils.h"
#include "ou.h"
#include "recorders.h"
#include "engine.h"

using namespace dynclamp;
using namespace dynclamp::recorders;

#define N_ENT 4

int main()
{
        SetLoggingLevel(Debug);

        int i;
        double t;
        double tend = 0.25;
        double sigma = 50;
        double tau = 10e-3;
        double i0 = 250;
        bool compress = false;
        std::vector<Entity*> entities(N_ENT);

        entities[0] = new H5Recorder(compress, "h5rec_test.h5");
        for (i=1; i<N_ENT; i++) {
                entities[i] = new OUcurrent(sigma, tau, i0, (double) i);
                entities[i]->connect(entities[0]);
        }

        Simulate(entities, tend);

        for (i=0; i<entities.size(); i++)
                delete entities[i];

        return 0;
}

