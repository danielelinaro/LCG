#include "neurons.h"
#include "currents.h"
#include "entity.h"
#include "engine.h"
#include "recorders.h"
#include <vector>

using namespace dynclamp;
using namespace dynclamp::recorders;
using namespace dynclamp::ionic_currents;

const double pi = 3.1415926536;

class FakeNeuron : public dynclamp::neurons::Neuron {
public:
        FakeNeuron(double V0, double f, double A, uint id = GetId())
                : Neuron(V0, id), m_V0(V0) {
                m_parameters["f"] = f;
                m_parameters["amp"] = A;
                setName("FakeNeuron");
                setUnits("mV");
        }
protected:
        virtual void evolve() {
                VM = m_V0 + m_parameters["amp"] * sin(2*pi*m_parameters["f"]*GetGlobalTime());
        }
private:
        double m_V0;
};

int main()
{
        std::vector<Entity*> entities(6);

        entities[0] = new HHSodium(1000);
        entities[1] = new HHPotassium(1000);
        entities[2] = new HHSodiumCN(1000);
        entities[3] = new HHPotassiumCN(1000);
        entities[4] = new FakeNeuron(-40, 1, 25);
        entities[5] = new H5Recorder(false, "currents.h5");

        for (uint i=0; i<4; i++) {
                entities[i]->connect(entities[4]);
                entities[i]->connect(entities[5]);
        }
        entities[4]->connect(entities[5]);

        Simulate(entities, 10);

        for (uint i=0; i<entities.size(); i++)
                delete entities[i];

        return 0;
}

