#include "currents.h"
#include "recorders.h"
#include "entity.h"
#include "engine.h"
#include "neurons.h"
#include "periodic_pulse.h"
#include <vector>

using namespace dynclamp;
using namespace dynclamp::recorders;
using namespace dynclamp::ionic_currents;
using namespace dynclamp::neurons;
using namespace dynclamp::generators;

int main()
{
        //SetLoggingLevel(Debug);

        std::vector<Entity*> entities(5);

        double area = 628.31853071795865;
        entities[0] = new ConductanceBasedNeuron(1, 0.0003, -54.3, 0, area, -20, -65);
        //entities[1] = new HHSodium(area);
        //entities[2] = new HHPotassium(area);
        entities[1] = new HHSodiumCN(area, 5061983);
        entities[2] = new HHPotassiumCN(area, 7051983);
        entities[3] = new PeriodicPulse(10, 1e-3, 30, 0.5, 10, 0.5, 1, 0.0);
        entities[4] = new H5Recorder(false, "pp.h5");

        entities[0]->connect(entities[4]);
        entities[1]->connect(entities[0]);
        entities[2]->connect(entities[0]);
        entities[3]->connect(entities[0]);
        entities[0]->connect(entities[3]);
        entities[3]->connect(entities[4]);

        Simulate(entities, 300);
        
        for (uint i=0; i<entities.size(); i++)
                delete entities[i];
        
        return 0;
}

