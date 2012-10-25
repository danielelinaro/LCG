#ifndef PERIODIC_PULSE
#define PERIODIC_PULSE

#include "generator.h"

#define PP_FREQUENCY    m_parameters["frequency"]
#define PP_DURATION     m_parameters["duration"]
#define PP_AMPLITUDE    m_parameters["amplitude"]
#define PP_PERIOD       m_parameters["period"]

namespace dynclamp {

namespace generators {

class PeriodicPulse : public Generator {
public:
        PeriodicPulse(double frequency, double duration, double amplitude, std::string units = "pA", uint id = GetId());

        virtual bool initialise();

        virtual bool hasNext() const;

        virtual void step();
        virtual double output();

        double period() const;

        void setFrequency(double frequency);
        void setPeriod(double period);
        void setAmplitude(double amplitude);
        void setDuration(double duration);

private:
        double m_output;
        double m_tNextPulse;
};

} // namespace generators

} // namespace dynclamp

/***
 *   FACTORY METHODS
 ***/
#ifdef __cplusplus
extern "C" {
#endif

dynclamp::Entity* PeriodicPulseFactory(string_dict& args);
        
#ifdef __cplusplus
}
#endif

#endif

