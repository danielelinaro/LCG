#ifndef NEURONS_H
#define NEURONS_H

#include "dynamical_entity.h"
#include "types.h"
#include "events.h"

#define VM       m_state[0]

#define LIF_C    m_parameters[0]
#define LIF_TAU  m_parameters[1]
#define LIF_TARP m_parameters[2]
#define LIF_ER   m_parameters[3]
#define LIF_E0   m_parameters[4]
#define LIF_VTH  m_parameters[5]
#define LIF_IEXT m_parameters[6]

//#define LIF_ARTIFICIAL_SPIKE

namespace dynclamp {

namespace neurons {

class Neuron : public DynamicalEntity {
public:
        Neuron(double Vm0, uint id = GetId(), double dt = GetGlobalDt());
        double Vm() const;
        virtual double output() const;
protected:
        void emitSpike(double timeout = 2e-3) const;
};

class LIFNeuron : public Neuron {
public:

        /**
         * \param C membrane capacitance.
         * \param tau membrane time constant.
         * \param tarp absolute refractory period.
         * \param Er reset voltage.
         * \param E0 resting potential.
         * \param Vth threshold.
         * \param Iext externally applied current.
         */
        LIFNeuron(double C, double tau, double tarp,
                  double Er, double E0, double Vth, double Iext,
                  uint id = GetId(), double dt = GetGlobalDt());

protected:
        virtual void evolve();

private:
        double m_tPrevSpike;
};

class RealNeuron : public Neuron {
};

} // namespace neurons

} // namespace dynclamp

#endif

