#include "pid.h"
#include "neurons.h"
#include "trigger.h"

dynclamp::Entity* PIDFactory(dictionary& args)
{
        uint id;
        double baseline, gp, gi, gd;
        id = dynclamp::GetIdFromDictionary(args);
        if ( ! dynclamp::CheckAndExtractDouble(args, "baselineCurrent", &baseline) ||
             ! dynclamp::CheckAndExtractDouble(args, "gp", &gp) ||
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
        : Entity(id)
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
                double errp, errd;
                errp = m_inputs[0] - m_inputs[1];
                m_erri += errp;
                errd = errp - m_errpPrev;
                m_errpPrev = errp;
                m_output = PID_BASELINE + PID_GP*errp + PID_GI*m_erri + PID_GD*errd;
                Logger(Info, "%9.3f %9.4f %9.4f %9.4f %7.2f\n", GetGlobalTime(), errp, m_erri, errd, m_output);                
        }
}

} // namespace dynclamp

