#include <math.h>
#include "conductance_stimulus.h"

lcg::Entity* ConductanceStimulusFactory(string_dict& args)
{
        uint id;
        double E;
        id = lcg::GetIdFromDictionary(args);
        if ( ! lcg::CheckAndExtractDouble(args, "E", &E)) {
                lcg::Logger(lcg::Critical, "Unable to build a conductance stimulus.\n");
                return NULL;
        }
        return new lcg::generators::ConductanceStimulus(E, id);
}

lcg::Entity* NMDAConductanceStimulusFactory(string_dict& args)
{
        uint id;
        double E, K1, K2;
        id = lcg::GetIdFromDictionary(args);
        if ( ! lcg::CheckAndExtractDouble(args, "E", &E) ||
             ! lcg::CheckAndExtractDouble(args, "K1", &K1) ||
             ! lcg::CheckAndExtractDouble(args, "K2", &K2)) {
                lcg::Logger(lcg::Critical, "Unable to build an NMDA conductance stimulus.\n");
                return NULL;
        }
        return new lcg::generators::NMDAConductanceStimulus(E, K1, K2, id);
}

namespace lcg {

namespace generators {

ConductanceStimulus::ConductanceStimulus(double E, uint id) : Generator(id), m_neuron(NULL)
{
        m_parameters["E"] = E;
        setName("ConductanceStimulus");
        setUnits("pA");
}

bool ConductanceStimulus::initialise()
{
        m_output = 0;
        return true;
}

bool ConductanceStimulus::hasNext() const
{
        return true;
}

void ConductanceStimulus::step()
{
        // conductances must be positive
        m_output = (!signbit(m_inputs[0])) * m_inputs[0] * (COND_E - m_neuron->output());
}

double ConductanceStimulus::output()
{
        return m_output;
}

void ConductanceStimulus::addPost(Entity *entity)
{
        Logger(Debug, "ConductanceStimulus::addPost(Entity*)\n");
        Entity::addPost(entity);
        neurons::Neuron *n = dynamic_cast<neurons::Neuron*>(entity);
        if (n != NULL) {
                Logger(Debug, "Connected to a neuron (id #%d).\n", entity->id());
                m_neuron = n;
        }
        else {
                Logger(Debug, "Entity #%d is not a neuron.\n", entity->id());
        }
}

NMDAConductanceStimulus::NMDAConductanceStimulus(double E, double K1, double K2, uint id)
        : ConductanceStimulus(E, id)
{
        m_parameters["K1"] = K1;
        m_parameters["K2"] = K2;
        setName("NMDAConductanceStimulus");
        setUnits("pA");
}

void NMDAConductanceStimulus::step()
{
        // conductances must be positive
        m_output = (!signbit(m_inputs[0])) * m_inputs[0] * (COND_E - m_neuron->output()) /
                (1 + NMDA_COND_K1*exp(-NMDA_COND_K2*m_neuron->output()));
}

} // namespace lcg

} // namespace generators

