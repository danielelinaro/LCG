#ifndef PERIODIC_PULSE
#define PERIODIC_PULSE

#include "generator.h"

#define PP_FREQUENCY    m_parameters[0]
#define PP_AMPLITUDE    m_parameters[1]
#define PP_PERIOD       m_parameters[2]

namespace dynclamp {

namespace generators {

class PeriodicPulse : public Generator {
public:
        PeriodicPulse(double frequency, double amplitude, uint id = GetId(), double dt = GetGlobalDt());

        virtual bool hasNext() const;

        virtual void step();
        virtual double output() const;

        double frequency() const;
        double period() const;
        double amplitude() const;
        void setFrequency(double frequency);
        void setPeriod(double period);
        void setAmplitude(double amplitude);

};

} // namespace generators

} // namespace dynclamp

/***
 *   FACTORY METHODS
 ***/
#ifdef __cplusplus
extern "C" {
#endif

dynclamp::Entity* PeriodicPulseFactory(dictionary& args);
        
#ifdef __cplusplus
}
#endif

#endif

