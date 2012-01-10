#include <cstdio>
#include <cstdlib>
#include <ctime>
#include "utils.h"
#include "synapses.h"
#include "events.h"

using namespace dynclamp;
using namespace dynclamp::synapses;

int main()
{
        double t, tspike;
        double E = 0.0;         // (mV)
        double dt;              // (s)
        double isi = 1.0/40;    // (s) -> 40 Hz
        int nisi = 50;          // number of spikes
        double dg = 1.0;        // (nS)
        int i, j;

        SpikeEvent event(NULL, 0.0);

        Synapse *syn[4];

        // Exponential synapse
        double expTau = 2e-3;    // (s)
        syn[0] = new ExponentialSynapse(E, dg, expTau);

        // Alpha synapse
        double alphaTau[2] = {0.5e-3,2e-3};     // (s)
        syn[1] = new Exp2Synapse(E, dg, alphaTau);

        // Facilitating TMG synapse
        double U_facil = 0.03;
        double tau_facil[3] = {3e-3, 100e-3, 1000e-3};         // tau_1, tau_recovery, tau_facilitation
        syn[2] = new TMGSynapse(E, dg, U_facil, tau_facil);

        // Depressing TMG synapse
        double U_depr = 0.5;
        double tau_depr[3] = {3e-3, 800e-3, 0e-3};              // tau_1, tau_recovery, tau_facilitation 
        syn[3] = new TMGSynapse(E, dg, U_depr, tau_depr);

        t = 0.0;
        dt = GetGlobalDt();
        for (int i=0; i<nisi; i++) {
                tspike = t + isi;
                while ((t=GetGlobalTime()) <= tspike) {
                        printf("%e", t);
                        for (j=0; j<4; j++) {
                                printf(" %e", syn[j]->g());
                                syn[j]->step();
                        }
                        printf("\n");
                        IncreaseGlobalTime();
                }
                for (j=0; j<4; j++)
                        syn[j]->handleEvent(&event);
        }

        tspike = t + 0.5;
        while ((t=GetGlobalTime()) <= tspike) {
                printf("%e", t);
                for (j=0; j<4; j++) {
                        printf(" %e", syn[j]->g());
                        syn[j]->step();
                }
                printf("\n");
                IncreaseGlobalTime();
        }

        for (j=0; j<4; j++)
                syn[j]->handleEvent(&event);

        while ((t=GetGlobalTime()) <= tspike + 0.3) {
                printf("%e", t);
                for (j=0; j<4; j++) {
                        printf(" %e", syn[j]->g());
                        syn[j]->step();
                }
                printf("\n");
                IncreaseGlobalTime();
        }

        for (j=0; j<4; j++)
                delete syn[j];

        return 0;
}

