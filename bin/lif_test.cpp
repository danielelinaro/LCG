#include <iostream>
#include "utils.h"
#include "neurons.h"

using namespace dynclamp;
using namespace dynclamp::neurons;

int main()
{
        double t, tend = 2, dt = GetGlobalDt();

        LIFNeuron lif(0.08, 0.0075, 0.0014, -65.2, -70, -50, 220);

        for (t=0.0; t<=tend; t+=dt) {
                std::cout << t << " " << lif.output() << std::endl;
                lif.step();
        }

        return 0;
}

