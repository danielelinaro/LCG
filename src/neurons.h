#ifndef NEURONS_H
#define NEURONS_H

#include "common.h"
#include "dynamical_entity.h"
#include "types.h"

#ifdef HAVE_CONFIG_H
#include "config.h"

#ifdef HAVE_LIBCOMEDI
#include <comedilib.h>
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#include "analog_io.h"
#include "aec.h"
#endif // HAVE_LIBCOMEDI
#endif // HAVE_CONFIG_H

#define VM       m_state[0]

#define IZH_V	m_state[0]
#define IZH_U	m_state[1]
#define IZH_A   m_parameters["a"]
#define IZH_B   m_parameters["b"]
#define IZH_C   m_parameters["c"]
#define IZH_D	m_parameters["d"]
#define IZH_VSPK	m_parameters["Vspk"]
#define IZH_IEXT	m_parameters["Iext"]


#define LIF_C      m_parameters["C"]
#define LIF_TAU    m_parameters["tau"]
#define LIF_TARP   m_parameters["tarp"]
#define LIF_ER     m_parameters["Er"]
#define LIF_E0     m_parameters["E0"]
#define LIF_VTH    m_parameters["Vth"]
#define LIF_IEXT   m_parameters["Iext"]
#define LIF_LAMBDA m_parameters["lambda"]
#define LIF_RL     m_parameters["Rl"]
#ifndef REALTIME_ENGINE
// I don't want to see ``fake'' spikes during real experiments.
#define LIF_ARTIFICIAL_SPIKE
#endif

#define CBN_VM_PREV             m_state[1]
#define CBN_C                   m_parameters["C"]
#define CBN_GL                  m_parameters["gl"]
#define CBN_EL                  m_parameters["El"]
#define CBN_IEXT                m_parameters["Iext"]
#define CBN_AREA                m_parameters["area"]
#define CBN_SPIKE_THRESH        m_parameters["thresh"]
#define CBN_GL_NS               m_parameters["gl_ns"]
#define CBN_COEFF               m_parameters["coeff"]

namespace dynclamp {

namespace neurons {

class Neuron : public DynamicalEntity {
public:
        Neuron(double Vm0, uint id = GetId());
        virtual bool initialise();
        double Vm() const;
        double Vm0() const;
        virtual double output();
protected:
        void emitSpike() const;
private:
        double m_Vm0;
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
                  uint id = GetId());
        virtual bool initialise();

protected:
        virtual void evolve();

private:
        double m_tPrevSpike;
};

class IzhikevichNeuron : public Neuron {
public:

        /**
		 * If v is the membrane voltage and u the membrane recovery variable:
         * \param a is the timescale of the recovery variable u (default 0.02).
		 * \param b is the coupling between u and v (default 0.2).
		 * \param c is the afterspike reset value of v (default -65).
         * \param d is the afterspike reset of u (default 2).
         * \param Vspk peak voltage of the spike (default 30mV).
         * \param Iext externally applied current.
		 * The parameters used in Izhikevich,2003 can reproduce a wide range of 
		 * spiking behaviours. Here follow some rough examples:
		 *		Excitatory neuron parameters
		 *		- Regular spiking: (0.02,0.2,-65,8,30)
		 *		- Intrinsic bursting: (0.02,0.2,-55,2,30)
		 *		- Chattering: (0.02,0.2,-50,2,30)
		 *		Inhibitory neuron parameters
		 *		- Fast spiking: (0.1,0.2,-65,8,30)
		 *		- Low threshold spiking: (0.1,0.25,-65,8,30)
		 *		Thalamic relay neuron parameters
		 *		- (0.02,0.2,-65,8,30)
		 *		Resonator (RZ):
		 *		- (0.1,0.26,-65,8,30)
         */
        IzhikevichNeuron(double a, double b, double c,
                  double d, double Vspk, double Iext,
                  uint id = GetId());
        virtual bool initialise();

protected:
        virtual void evolve();

private:
        double m_tPrevSpike;
};

class ConductanceBasedNeuron : public Neuron {
public:
        /**
         * \param C membrane capacitance (uF/cm^2)
         * \param gl leak conductance (S/cm^2)
         * \param El leak reversal potential (mV)
         * \param Iext externally applied current (pA)
         * \param area surface of the membrane (um^2)
         * \param spikeThreshold threshold for emitting a spike (mV)
         * \param V0 initial value of the membrane potential (mV)
         */
        ConductanceBasedNeuron(double C, double gl, double El, double Iext,
                               double area, double spikeThreshold, double V0,
                               uint id = GetId());
protected:
        virtual void evolve();
};

#ifdef HAVE_LIBCOMEDI

#define RN_VM_PREV              m_state[1]
#define RN_SPIKE_THRESH         m_parameters["thresh"]

class RealNeuron : public Neuron {
public:
        RealNeuron(double spikeThreshold, double V0,
                   const char *deviceFile,
                   uint inputSubdevice, uint outputSubdevice,
                   uint readChannel, uint writeChannel,
                   double inputConversionFactor, double outputConversionFactor,
                   uint inputRange, uint reference, const char *kernelFile = NULL,
                   bool holdLastValue = false, bool adaptiveThreshold = false, uint id = GetId());

        RealNeuron(double spikeThreshold, double V0,
                   const char *deviceFile,
                   uint inputSubdevice, uint outputSubdevice,
                   uint readChannel, uint writeChannel,
                   double inputConversionFactor, double outputConversionFactor,
                   uint inputRange, uint reference, 
                   const double *AECKernel, size_t kernelSize, bool holdLastValue = false,
                   bool adaptiveThreshold = false, uint id = GetId());

        ~RealNeuron();

        virtual bool initialise();
        virtual void terminate();

        virtual bool hasMetadata(size_t *ndims) const;
        virtual const double* metadata(size_t *dims, char *label) const;

protected:
        virtual void evolve();

private:
        AEC m_aec;
        ComediAnalogInputSoftCal  m_input;
        ComediAnalogOutputSoftCal m_output;
        bool m_holdLastValue;
        // injected current
        double m_Iinj;

        // adaptive threshold variables
        bool m_adaptiveThreshold;
        double m_Vmax, m_Vmin, m_Vth;

};
#endif // HAVE_LIBCOMEDI

} // namespace neurons

} // namespace dynclamp


/***
 *   FACTORY METHODS
 ***/
#ifdef __cplusplus
extern "C" {
#endif

dynclamp::Entity* LIFNeuronFactory(string_dict& args);
dynclamp::Entity* IzhikevichNeuronFactory(string_dict& args);
dynclamp::Entity* ConductanceBasedNeuronFactory(string_dict& args);
#ifdef HAVE_LIBCOMEDI
dynclamp::Entity* RealNeuronFactory(string_dict& args);
#endif
	
#ifdef __cplusplus
}
#endif

#endif

