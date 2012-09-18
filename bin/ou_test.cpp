#include <vector>
#include <string>
#include <sstream>
#include "utils.h"
#include "ou.h"
#include "recorders.h"
#include "engine.h"
#include "randlib.h"

using namespace dynclamp;
using namespace dynclamp::recorders;

#define N_ENT 11

int main()
{
        int i;
        double tend = 5;
        string_dict params;
        std::vector<Entity*> entities(N_ENT);
        params["compress"] = "false";
        params["filename"] = "ou_test.h5";
        entities[0] = EntityFactory("H5Recorder", params);
        for (i=1; i<N_ENT; i++) {
                std::stringstream ss;
                ss << GetRandomSeed();
                params.clear();
                params["sigma"] = "50";
                params["tau"] = "0.01";
                params["I0"] = "250";
                params["seed"] = ss.str();
                params["interval"] = "1,4";
                entities[i] = EntityFactory("OUcurrent", params);
                entities[i]->connect(entities[0]);
        }
        Simulate(entities, tend);
        for (i=0; i<N_ENT; i++)
                delete entities[i];
        return 0;
}

