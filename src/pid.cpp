#include "pid.h"
#include "neurons.h"
#include "trigger.h"

dynclamp::Entity* PIDFactory(dictionary& args)
{
        uint id;
        double baseline, gp, gi, gd;
        id = dynclamp::GetIdFromDictionary(args);
        if ( ! dynclamp::CheckAndExtractDouble(args, "gp", &gp) ||
             ! dynclamp::CheckAndExtractDouble(args, "gi", &gi)) {
                dynclamp::Logger(dynclamp::Critical, "Unable to build a PID.\n");
                return NULL;
        }
        if (! dynclamp::CheckAndExtractDouble(args, "gd", &gd))
                gd = 0.0;
        return new dynclamp::PID(baseline, gp, gi, gd, id);
        
}

namespace dynclamp {

PID::PID(double baseline, double gp, double gi, double gd, uint id)
        : Entity(id), m_cnt(0)
{
        m_parameters.push_back(baseline);
        m_parameters.push_back(gp);
        m_parameters.push_back(gi);
        m_parameters.push_back(gd);
}

double PID::output() const
{
        return m_output;
}

void PID::initialise()
{
        m_output = PID_BASELINE;
        m_erri = 0.0;
        m_errpPrev = 0.0;
}

void PID::step()
{}

void PID::handleEvent(const Event *event)
{
        if (event->type() == SPIKE || event->type() == TRIGGER) {
                Logger(Debug, "Using inputs #%d and #%d to compute the error.\n", m_idx[0], m_idx[1]);
                double errp, errd;
                errp = m_inputs[m_idx[0]] - m_inputs[m_idx[1]];
                m_erri += errp;
                errd = errp - m_errpPrev;
                m_errpPrev = errp;
                m_output = PID_BASELINE + PID_GP*errp + PID_GI*m_erri + PID_GD*errd;
                Logger(Info, "%g %g %g\n", GetGlobalTime(), errp, m_output);                
        }
}

void PID::addPre(Entity *entity)
{
        Entity::addPre(entity);
        if (dynamic_cast<neurons::Neuron*>(entity) == NULL && dynamic_cast<Trigger*>(entity) == NULL) {
                Logger(Debug, "Entity #%d is neither a Neuron nor a Trigger.\n", entity->id());
                m_idx[m_cnt] = m_inputs.size() - 1;
                m_cnt = (m_cnt + 1) % 2;
        }
}

} // namespace dynclamp

