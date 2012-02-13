#include <vector>
#include "engine.h"
#include "entity.h"
#include "recorders.h"
#include "periodic_pulse.h"

using namespace dynclamp;
using namespace dynclamp::recorders;
using namespace dynclamp::generators;

int main()
{
        std::vector<Entity*> entities(2);
        entities[0] = new PeriodicPulse(1, 1, 0.05);
        entities[1] = new H5Recorder(false, "pp.h5");

        entities[0]->connect(entities[1]);
        Simulate(entities, 15.75);

        delete entities[0];
        delete entities[1];

        return 0;
}

