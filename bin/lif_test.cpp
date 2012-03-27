#include <vector>
#include "entity.h"
#include "utils.h"

using namespace dynclamp;

int main()
{
        double tend, dt;
        std::vector<Entity*> entities;
        ParseConfigurationFile("configs/lif.xml", entities, &tend, &dt);
        SetGlobalDt(dt);
        Simulate(entities, tend);
        for (int i=0; i<entities.size(); i++)
                delete entities[i];
        return 0;
}

