#ifndef NEURONS_H
#define NEURONS_H

#include "dynamical_entity.h"
#include "types.h"

#ifdef HAVE_LIBCOMEDI
#include <comedilib.h>
#endif

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
        void emitSpike() const;
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

#ifdef HAVE_LIBCOMEDI

#define RN_VM_PREV                  m_state[1]

#define RN_SPIKE_THRESH             m_parameters[0]

class RealNeuron : public Neuron {
public:
        RealNeuron(const char *kernelFile,
                   const char *deviceFile,
                   uint inputSubdevice, uint outputSubdevice,
                   uint readChannel, uint writeChannel,
                   double inputConversionFactor = 100, double outputConversionFactor = 0.0025,
                   double spikeThreshold = -20);

        RealNeuron(const double *AECKernel, size_t kernelSize,
                   const char *deviceFile,
                   uint inputSubdevice, uint outputSubdevice,
                   uint readChannel, uint writeChannel,
                   double inputConversionFactor = 100, double outputConversionFactor = 0.0025,
                   double spikeThreshold = -20);

        virtual ~RealNeuron();

protected:
        virtual void addPre(Entity *entity, double input);

private:
        ComediAnalogInput  m_analogInput;
        ComediAnalogOutput m_analogOutput;
        AEC m_aec;
};
#endif

} // namespace neurons

} // namespace dynclamp

#endif

