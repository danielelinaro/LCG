#include "currents.h"
#include "recorders.h"
#include "entity.h"
#include "engine.h"
#include "neurons.h"
#include "stimulus_generator.h"
#include "pid.h"
#include "frequency_estimator.h"
#include <vector>

using namespace dynclamp;
using namespace dynclamp::recorders;
using namespace dynclamp::ionic_currents;
using namespace dynclamp::neurons;
using namespace dynclamp::generators;

int main()
{
        uint i, n;
        SetGlobalDt(1./20000);

        std::vector<Entity*> entities(8);

        double area = 628.31853071795865;
        entities[0] = new ConductanceBasedNeuron(1, 0.0003, -54.3, 0, area, -20, -65);
        entities[1] = new HHSodium(area);
        entities[2] = new HHPotassium(area);
        //entities[1] = new HHSodiumCN(area, 5061983);
        //entities[2] = new HHPotassiumCN(area, 7051983);
        entities[3] = new Waveform("ou.stim");
        entities[4] = new Waveform("freq.stim");
        entities[5] = new FrequencyEstimator(0.1);
        entities[6] = new PID(60, 0.2, 0.1, 0.0);
        entities[7] = new H5Recorder(false, "freq_clamp.h5");

        n = entities.size();

        // neuron to frequency estimator
        entities[0]->connect(entities[5]);
        // neuron to PID
        entities[0]->connect(entities[6]);

        // sodium current to neuron
        entities[1]->connect(entities[0]);
        
        // potassium current to neuron
        entities[2]->connect(entities[0]);
        
        // noisy current to neuron
        entities[3]->connect(entities[0]);
        
        // frequency waveform to PID
        entities[4]->connect(entities[6]);
        
        // estimated frequency to PID
        entities[5]->connect(entities[6]);
        
        // PID to neuron
        entities[6]->connect(entities[0]);

        for (i=0; i<n-1; i++)
                entities[i]->connect(entities[n-1]);

        Simulate(entities, dynamic_cast<Waveform*>(entities[4])->duration());
        
        for (i=0; i<n; i++)
                delete entities[i];
        
        return 0;
}

