#include <cstdio>
#include "utils.h"
#include "neurons.h"

using namespace dynclamp;
using namespace dynclamp::neurons;

int main()
{
        double t, tend = 2;

        LIFNeuron lif(0.08, 0.0075, 0.0014, -65.2, -70, -50, 220);

        while ((t = GetGlobalTime()) <= tend) {
                printf("%e %e\n", t, lif.output());
                IncreaseGlobalTime();
                lif.step();
        }

        return 0;
}

