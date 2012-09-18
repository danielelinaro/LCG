#ifndef PERIODIC_PULSE
#define PERIODIC_PULSE

#include "generator.h"

#define PP_FREQUENCY    m_parameters["f"]
#define PP_DURATION     m_parameters["dur"]
#define PP_AMPLITUDE    m_parameters["amp"]
#define PP_PERIOD       m_parameters["T"]

#define PP_INTERVAL     15e-3

namespace dynclamp {

namespace generators {

class PeriodicPulse : public Generator {
public:
        PeriodicPulse(double frequency, double duration, double amplitude, uint id = GetId());

        virtual bool initialise();

        virtual bool hasNext() const;

        virtual void step();
        virtual double output();

        void handleEvent(const Event *event);

        double period() const;

        void setFrequency(double frequency);
        void setPeriod(double period);
        void setAmplitude(double amplitude);
        void setDuration(double duration);

private:
        double m_period;
        double m_output;
        double m_amplitude;
        double m_tLastPulse;
        double m_tNextPulse;
        double m_tUpdate;
        double m_tLastSpike;
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

