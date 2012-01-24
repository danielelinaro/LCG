#include <cstdio>>
#include "utils.h"
#include "ou.h"
#include "recorders.h"
#include "engine.h"
#include "randlib.h"

using namespace dynclamp;
using namespace dynclamp::recorders;

#define N_ENT 11

int main()
{
        {
                const int n = 10000;
                int data[n];
                shuffle(-5000,n-50001,data);
                for (int i=0; i<n; i++)
                        fprintf(stderr, "%d\n", data[i]);
        }

        int i;
        double tend = 5;
        double sigma = 50;
        double tau = 10e-3;
        double i0 = 250;
        std::vector<Entity*> entities(N_ENT);
        entities[0] = new ASCIIRecorder("ou_test.out");
        for (i=1; i<N_ENT; i++) {
                entities[i] = new OUcurrent(sigma, tau, i0, (double) i);
                entities[i]->connect(entities[0]);
        }

        Simulate(entities, tend);

        for (i=0; i<N_ENT; i++)
                delete entities[i];

        return 0;
}

