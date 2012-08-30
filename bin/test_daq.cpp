#include <stdio.h>
#include <vector>
#include "entity.h"
#include "analog_io.h"
#include "waveform.h"
#include "recorders.h"
#include "engine.h"
#include "utils.h"
#include "delay.h"
#include "time_logger.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define HEKA
#define TWO_CHANNELS

#ifdef HAVE_LIBCOMEDI
#include "comedi_io.h"
#endif

using namespace dynclamp;
using namespace dynclamp::generators;
using namespace dynclamp::recorders;

int main()
{
#ifdef HAVE_LIBCOMEDI

        SetGlobalDt(1./20000);

#ifdef HEKA
        uint reference = GRSE;
#else
        uint reference = NRSE;
#endif // HEKA

        std::vector<Entity*> entities;

        try {
                // AO0
                entities.push_back( new AnalogOutput("/dev/comedi0", 1, 0, 1., reference) );
#ifdef HEKA
                // AI0
                entities.push_back( new AnalogInput("/dev/comedi0", 0, 0, 1., PLUS_MINUS_TEN, reference) );
#else
                // AI2
                entities.push_back( new AnalogInput("/dev/comedi0", 0, 2, 1., PLUS_MINUS_TEN, reference) );
#endif // HEKA
                entities.push_back( new Waveform("positive-step.stim") );
                // connect analog output to analog input
                Logger(Info, "Connecting the first analog output to the first analog input.\n");
                entities[0]->connect(entities[1]);
                // connect the stimulus to AO0
                Logger(Info, "Connecting the stimulus to the first analog output.\n");
                entities.back()->connect(entities[0]);

#ifdef TWO_CHANNELS
                // AO1
                entities.push_back( new AnalogOutput("/dev/comedi0", 1, 1, 1., reference) );
#ifdef HEKA
                // AI1
                entities.push_back( new AnalogInput("/dev/comedi0", 0, 1, 1., PLUS_MINUS_TEN, reference) );
#else
                // AI3
                entities.push_back( new AnalogInput("/dev/comedi0", 0, 3, 1., PLUS_MINUS_TEN, reference) );
#endif // HEKA
                // connect analog output to ananlog input
                Logger(Info, "Connecting the second analog output to the second analog input.\n");
                entities[3]->connect(entities[4]);
                entities.push_back( new Waveform("negative-step.stim") );
                // connect the stimulus to AO1
                Logger(Info, "Connecting the stimulus to the second analog output.\n");
                entities.back()->connect(entities[2]);
#endif // TWO_CHANNELS

                entities.push_back( new TimeLogger() );
                entities.push_back( new H5Recorder(false, "daq_test.h5") );
        
                // connect entities to the recorder
                for (uint i=0; i<entities.size()-1; i++)
                        entities[i]->connect(entities.back());

                Simulate(entities, 2.0);

        } catch (const char* msg) {
                fprintf(stderr, "Error: %s.\n", msg);
        }

        for (uint i=0; i<entities.size(); i++)
                delete entities[i];

#else
        Logger(Critical, "This program needs COMEDI.\n");
#endif // HAVE_LIBCOMEDI

        return 0;
}

