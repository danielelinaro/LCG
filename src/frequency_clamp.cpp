#include "frequency_clamp.h"
#include <math.h>

dynclamp::Entity* FrequencyClampFactory(dictionary& args)
{        
        uint id;
        double frequency, baselineCurrent, tau, gp, gi, gd;

        id = dynclamp::GetIdFromDictionary(args);

        if ( ! dynclamp::CheckAndExtractDouble(args, "baselineCurrent", &baselineCurrent) ||
             ! dynclamp::CheckAndExtractDouble(args, "tau", &tau) ||
             ! dynclamp::CheckAndExtractDouble(args, "gp", &gp) ||
             ! dynclamp::CheckAndExtractDouble(args, "gi", &gi) ||
             ! dynclamp::CheckAndExtractDouble(args, "gd", &gd)) {
                dynclamp::Logger(dynclamp::Critical, "Unable to build FrequencyClamp.\n");
                return NULL;
        }

        if ( ! dynclamp::CheckAndExtractDouble(args, "frequency", &frequency))
                return new dynclamp::generators::FrequencyClamp(baselineCurrent, tau, gp, gi, gd, id);

        return new dynclamp::generators::FrequencyClamp(frequency, baselineCurrent, tau, gp, gi, gd, id);

}

namespace dynclamp {

namespace generators {

FrequencyClamp::FrequencyClamp(double baselineCurrent, double tau,
                double gp, double gi, double gd, uint id)
        : Generator(id), m_constantFrequency(false), m_generator(NULL)
{
        m_parameters.push_back(baselineCurrent);// m_parameters[0] -> baseline current
        m_parameters.push_back(tau);            // m_parameters[1] -> time constant
        m_parameters.push_back(gp);             // m_parameters[2] -> proportional gain
        m_parameters.push_back(gi);             // m_parameters[3] -> integral gain
        m_parameters.push_back(gd);             // m_parameters[4] -> derivative gain
        m_parametersNames.push_back("baselineCurrent");
        m_parametersNames.push_back("tau");
        m_parametersNames.push_back("gp");
        m_parametersNames.push_back("gi");
        m_parametersNames.push_back("gd");
        setName("FrequencyClamp");
        setUnits("pA");
}

FrequencyClamp::FrequencyClamp(double frequency, double baselineCurrent, double tau,
                double gp, double gi, double gd, uint id)
        : Generator(id), m_constantFrequency(true), m_generator(NULL)
{
        m_parameters.push_back(baselineCurrent);// m_parameters[0] -> baseline current
        m_parameters.push_back(tau);            // m_parameters[1] -> time constant
        m_parameters.push_back(gp);             // m_parameters[2] -> proportional gain
        m_parameters.push_back(gi);             // m_parameters[3] -> integral gain
        m_parameters.push_back(gd);             // m_parameters[4] -> derivative gain
        m_parameters.push_back(frequency);      // m_parameters[5] -> frequency
        m_parametersNames.push_back("baselineCurrent");
        m_parametersNames.push_back("tau");
        m_parametersNames.push_back("gp");
        m_parametersNames.push_back("gi");
        m_parametersNames.push_back("gd");
        m_parametersNames.push_back("frequency");
        setName("FrequencyClamp");
        setUnits("pA");
}

void FrequencyClamp::addPre(Entity *entity)
{
        Entity::addPre(entity);
        Generator *g = dynamic_cast<Generator*>(entity);
        if (g != NULL) {
                Logger(Debug, "A generator has been connected (id #%d).\n", entity->id());
                m_generator = g;
        }
        else {
                Logger(Debug, "Entity #%d is not a generator.\n", entity->id());
        }
}

bool FrequencyClamp::hasNext() const
{
        return true;
}

bool FrequencyClamp::initialise()
{
        m_estimatedFrequency = 0.0;
        m_tPrevSpike = 0.0;
        m_errp = 0.0;
        m_erri = 0.0;
        m_errd = 0.0;
        m_errorpPrev = 0.0;
        m_current = FC_BASELINE;
        return true;
}

void FrequencyClamp::step()
{}

double FrequencyClamp::output() const
{
        return m_current;
}

void FrequencyClamp::handleEvent(const Event *event)
{
        double now = event->time();
        if (m_tPrevSpike > 0) {
                double isi, weight;
                isi = now - m_tPrevSpike;
                weight = exp(-isi/FC_TAU);
                m_estimatedFrequency = (1-weight)/isi + weight*m_estimatedFrequency;
                if (m_constantFrequency)
                        m_errp = FC_F - m_estimatedFrequency;
                else
                        // a waveform generator must be connected to this entity
                        m_errp = m_generator->output() - m_estimatedFrequency;
                m_erri += m_errp;
                m_errd = m_errp - m_errorpPrev;
                m_errorpPrev = m_errp;
                m_current = FC_BASELINE + FC_GP*m_errp + FC_GI*m_erri + FC_GD*m_errd;
                Logger(Debug, "%g %g %g %g\n", now, m_errp, m_current, m_estimatedFrequency);
        }
        m_tPrevSpike = now;
}

} // namespace generators

} // namespace dynclamp
