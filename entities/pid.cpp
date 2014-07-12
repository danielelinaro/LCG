#include "pid.h"

lcg::Entity* PIDFactory(string_dict& args)
{
        uint id;
        double baseline, gp, gi, gd;
        id = lcg::GetIdFromDictionary(args);
        if ( ! lcg::CheckAndExtractDouble(args, "baselineCurrent", &baseline) ||
             ! lcg::CheckAndExtractDouble(args, "gp", &gp) ||
             ! lcg::CheckAndExtractDouble(args, "gi", &gi)) {
                lcg::Logger(lcg::Critical, "PID(%d): Unable to build. Need to specify baselineCurrent, gp, and gi.\n", id);
                return NULL;
        }
        if (! lcg::CheckAndExtractDouble(args, "gd", &gd))
                gd = 0.0;
        return new lcg::PID(baseline, gp, gi, gd, id);
        
}

namespace lcg {

PID::PID(double baseline, double gp, double gi, double gd, uint id)
        : Entity(id)
{
        PID_BASELINE = baseline;
        PID_GP = gp;
        PID_GI = gi;
        PID_GD = gd;
        setName("PID");
}

double PID::output()
{
        return m_output;
}

bool PID::state(){
        return m_state;
}

void PID::changeState(){
        m_state = !m_state;
}

bool PID::initialise()
{
        m_output = PID_BASELINE;
        m_erri = 0.0;
        m_errpPrev = 0.0;
        m_state = true;
        Logger(Info, "PID(%d): %s %s %s %s %s\n", id(), "Time","FirstInput","SecondInput","Perror", "Ierror", "Derror", "Output");                
        return true;
}

void PID::step()
{}

void PID::handleEvent(const Event *event)
{   
    switch(event->type())
    {
        case SPIKE:
        case TRIGGER:
            if(m_state) {
                double errp, errd;
                errp = m_inputs[0] - m_inputs[1];
                m_erri += errp;
                errd = errp - m_errpPrev;
                m_errpPrev = errp;
                m_output = PID_BASELINE + PID_GP*errp + PID_GI*m_erri + PID_GD*errd;

                Logger(Info, "PID(%d): %9.3f %9.3f %9.3f %9.4f %9.4f %9.4f %10.5f\n", id(), GetGlobalTime(),m_inputs[0], m_inputs[1], errp, m_erri, errd, m_output);                
            }
            break;
        case TOGGLE:
            changeState();
			Logger(Debug, "PID(%d): Toggled at %9.3f.\n", id(), GetGlobalTime()); 
            break;
    }
}

} // namespace lcg

