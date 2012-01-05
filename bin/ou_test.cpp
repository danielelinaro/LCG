#include <cstdio>>
#include "utils.h"
#include "ou.h"

using namespace dynclamp;

int main()
{
        double t;
        double tend = 50;
        double sigma = 50;
        double tau = 10e-3;
        double i0 = 250;
        OUcurrent ou(sigma, tau, i0);

        while((t=GetGlobalTime()) <= tend) {
                printf("%e %e\n", t, ou.output());
                IncreaseGlobalTime();
                ou.step();
        }

        return 0;
}

