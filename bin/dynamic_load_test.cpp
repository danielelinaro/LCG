#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "utils.h"
#include "engine.h"
#include "entity.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

using namespace dynclamp;

int main()
{
        double tend;
        std::vector<Entity*> entities;
        dictionary parameters;

        SetLoggingLevel(Critical);

#ifdef HAVE_LIBCOMEDI
        tend = 9.0;     // duration of the stimulus

        parameters["filename"] = "recording.h5";
        parameters["compress"] = "false";
        entities.push_back( EntityFactory("H5Recorder", parameters) );
        
        parameters.clear();
        parameters["deviceFile"] = "/dev/comedi0";
        parameters["inputSubdevice"] = "0";
        parameters["readChannel"] = "2";
        parameters["inputConversionFactor"] = "20";
        entities.push_back( EntityFactory("AnalogInput", parameters) );

        parameters.clear();
        parameters["deviceFile"] = "/dev/comedi0";
        parameters["outputSubdevice"] = "1";
        parameters["writeChannel"] = "1";
        parameters["outputConversionFactor"] = "0.0025";
        entities.push_back( EntityFactory("AnalogOutput", parameters) );

        parameters.clear();
        parameters["filename"] = "example.stim";
        entities.push_back( EntityFactory("Stimulus", parameters) );

        Logger(Debug, "Connecting the analog input to the recorder.\n");
        entities[1]->connect(entities[0]);      // connect the analog input to the recorder
        Logger(Debug, "Connecting the analog output to the recorder.\n");
        entities[2]->connect(entities[0]);      // connect the analog output to the recorder (save the injected current)
        Logger(Debug, "Connecting the stimulus to the analog output.\n");
        entities[3]->connect(entities[2]);      // connect the stimulus to the analog output

#else
        tend = 5;

        parameters["filename"] = "lif.h5";
        parameters["compress"] = "false";
        entities.push_back( EntityFactory("H5Recorder", parameters) );
        
        parameters.clear();
        parameters["C"]    = "0.08";
        parameters["tau"]  = "0.0075";
        parameters["tarp"] = "0.0014";
        parameters["Er"]   = "-65.2";
        parameters["E0"]   = "-70.0";
        parameters["Vth"]  = "-50.0";
        parameters["Iext"] = "220.0";
        entities.push_back( EntityFactory("LIFNeuron", parameters) );

        entities[1]->connect(entities[0]);
#endif

        Simulate(entities, tend);

        for (uint i=0; i<entities.size(); i++)
                delete entities[i];

        return 0;
}

