#include <cstdio>
#include <vector>
#include "utils.h"
#include "engine.h"
#include "entity.h"
#include "recorders.h"
#include "stimulus_generator.h"

using namespace dynclamp;
using namespace dynclamp::recorders;
using namespace dynclamp::generators;

int main(int argc, char *argv[])
{
        if (argc != 2) {
                Logger(Critical, "Usage: %s stimfile\n", argv[0]);
                return 1;
        }

        std::vector<Entity*> entities(2);
        entities[0] = new Stimulus(argv[1]);
        entities[1] = new H5Recorder("stimulus.h5", false);
        entities[0]->connect(entities[1]);

        Simulate(entities, dynamic_cast<Stimulus*>(entities[0])->duration());

        delete entities[0];
        delete entities[1];

        return 0;
}

