#ifndef PERIODIC_PULSE
#define PERIODIC_PULSE

#include "generator.h"

#define PP_FREQUENCY    m_parameters[0]
#define PP_DURATION     m_parameters[1]
#define PP_AMPLITUDE    m_parameters[2]
#define PP_PERIOD       m_parameters[3]
#define PP_PROB         m_parameters[4]
#define PP_TAU          m_parameters[5]
#define PP_GP           m_parameters[6]
#define PP_GI           m_parameters[7]
#define PP_GD           m_parameters[8]

#define PP_INTERVAL     15e-3

namespace dynclamp {

namespace generators {

class PeriodicPulse : public Generator {
public:
        PeriodicPulse(double frequency, double duration, double amplitude, uint id = GetId());
        PeriodicPulse(double frequency, double duration, double amplitude, double probability,
                      double tau, double gp, double gi = 0.0, double gd = 0.0,
                      uint id = GetId());

        virtual void initialise();

        virtual bool hasNext() const;

        virtual void step();
        virtual double output() const;

        void handleEvent(const Event *event);

        double frequency() const;
        double period() const;
        double amplitude() const;
        double duration() const;
        void setFrequency(double frequency);
        void setPeriod(double period);
        void setAmplitude(double amplitude);
        void setDuration(double duration);

private:
        double m_output, m_amplitude;
        double m_tLastPulse, m_tNextPulse, m_tUpdate, m_tLastSpike;
        bool   m_clamp;
        double m_estimatedProbability;
        double m_errp, m_erri, m_errd, m_errpPrev;
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

