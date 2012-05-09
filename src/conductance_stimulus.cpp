#include "conductance_stimulus.h"

dynclamp::Entity* ConductanceStimulusFactory(dictionary& args)
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

ConductanceStimulus::ConductanceStimulus(double E, uint id) : Generator(id)
{
        m_parameters.push_back(E);
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
        m_output = m_inputs[0] * (COND_E - m_neuron->output());
}

double ConductanceStimulus::output() const
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

