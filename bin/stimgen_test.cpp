#include <cstdio>
#include <cstdlib>
#include "stimulus_generator.h"
#include "utils.h"

using namespace dynclamp;
using namespace dynclamp::generators;

int main(int argc, char *argv[])
{
        if (argc != 2) {
                fprintf(stderr, "Usage: %s stimfile\n", argv[0]);
                exit(1);
        }

        double t;
        Stimulus stim(argv[1]);

        while (stim.hasNext()) {
                fprintf(stdout, "%e %e\n", GetGlobalTime(), stim.output());
                stim.step();
                IncreaseGlobalTime();
        }

        return 0;
}

