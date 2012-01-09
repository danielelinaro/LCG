#include <cstdio>>
#include "utils.h"
#include "ou.h"
#include "recorders.h"

using namespace dynclamp;
using namespace dynclamp::recorders;

#define N_ENT 4

int main()
{
        int i;
        double t;
        double tend = 0.25;
        double sigma = 50;
        double tau = 10e-3;
        double i0 = 250;
        bool compress = false;
        Entity *entities[N_ENT];
        entities[0] = new H5Recorder("h5rec_test.h5", compress);
        for (i=1; i<N_ENT; i++) {
                entities[i] = new OUcurrent(sigma, tau, i0, (double) i);
                entities[i]->connect(entities[0]);
        }

        try {
                while((t=GetGlobalTime()) <= tend) {
                        for (i=0; i<N_ENT; i++)
                                entities[i]->readAndStoreInputs();
                        IncreaseGlobalTime();
                        for (i=0; i<N_ENT; i++)
                                entities[i]->step();
                }
        } catch (const char *msg) {
                fprintf(stderr, "ERROR: %s\n", msg);
        }

        for (i=0; i<N_ENT; i++)
                delete entities[i];

        return 0;
}

