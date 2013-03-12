#include <math.h>
#include "conductance_stimulus.h"

dynclamp::Entity* ConductanceStimulusFactory(string_dict& args)
{
        uint id;
        double E;
        id = dynclamp::GetIdFromDictionary(args);
        if ( ! dynclamp::CheckAndExtractDouble(args, "E", &E)) {
                dynclamp::Logger(dynclamp::Critical, "Unable to build a conductance stimulus.\n");
                return NULL;
        }
        return new dynclamp::generators::ConductanceStimulus(E, id);
}

namespace dynclamp {

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


} // namespace dynclamp

} // namespace generators

