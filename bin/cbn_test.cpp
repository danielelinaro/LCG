#include "currents.h"
#include "recorders.h"
#include "entity.h"
#include "engine.h"
#include "neurons.h"
#include <vector>

using namespace dynclamp;
using namespace dynclamp::recorders;
using namespace dynclamp::ionic_currents;
using namespace dynclamp::neurons;

int main()
{
        std::vector<Entity*> entities(4);

        entities[0] = new ConductanceBasedNeuron(1, 0.0003, -54.3, 0, 100, -20, -65);
        entities[1] = new HHSodium(100);
        entities[2] = new HHPotassium(100);
        entities[3] = new H5Recorder(false, "cbn.h5");

        entities[0]->connect(entities[3]);

        entities[1]->connect(entities[0]);
        entities[1]->connect(entities[3]);
        
        entities[2]->connect(entities[0]);
        entities[2]->connect(entities[3]);

        Simulate(entities, 10);
        
        for (uint i=0; i<entities.size(); i++)
                delete entities[i];
        
        return 0;
}

