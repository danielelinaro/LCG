#include "types.h"
#include "utils.h"
#include "engine.h"
#include "entity.h"

using namespace dynclamp;

int main()
{
        double t, tend = 2;
        std::vector<Entity*> entities;
        dictionary parameters;

        parameters["C"]    = "0.08";
        parameters["tau"]  = "0.0075";
        parameters["tarp"] = "0.0014";
        parameters["Er"]   = "-65.2";
        parameters["E0"]   = "-70.0";
        parameters["Vth"]  = "-50.0";
        parameters["Iext"] = "220.0";

        entities.push_back( EntityFactory("LIFNeuron", parameters) );

        while ((t = GetGlobalTime()) <= tend) {
                printf("%e %e\n", t, entities[0]->output());
                IncreaseGlobalTime();
                entities[0]->step();
        }
        
        //Simulate(entities, 10);

        delete entities[0];

        return 0;
}

