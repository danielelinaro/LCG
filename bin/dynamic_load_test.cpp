#include <cstdio>
#include "types.h"
#include "utils.h"
#include "engine.h"
#include "entity.h"

using namespace dynclamp;

int main()
{
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

        parameters.clear();
        parameters["filename"] = "lif.h5";
        parameters["compress"] = "false";

        entities.push_back( EntityFactory("H5Recorder", parameters) );
        
        entities[0]->connect(entities[1]);

        Simulate(entities, 5);

        delete entities[0];
        delete entities[1];

        return 0;
}

