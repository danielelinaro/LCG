#include "neurons.h"
#include "events.h"
#include "thread_safe_queue.h"

namespace dynclamp {

extern ThreadSafeQueue<Event*> eventsQueue;

namespace neurons {

Neuron::Neuron(double Vm0, uint id, double dt)
        : DynamicalEntity(id, dt)
{
        m_state.push_back(Vm0); // m_state[0] -> membrane potential
}

double Neuron::Vm() const
{
        return VM;
}

double Neuron::output() const
{
        return VM;
}

void Neuron::emitSpike() const
{
        emitEvent(new SpikeEvent(this));
}

LIFNeuron::LIFNeuron(double C, double tau, double tarp,
                     double Er, double E0, double Vth, double Iext,
                     uint id, double dt)
        : Neuron(E0, id, dt), m_tPrevSpike(-1000.0)
{
        m_parameters.push_back(C);
        m_parameters.push_back(tau);
        m_parameters.push_back(tarp);
        m_parameters.push_back(Er);
        m_parameters.push_back(E0);
        m_parameters.push_back(Vth);
        m_parameters.push_back(Iext);
        m_parameters.push_back(m_dt/LIF_TAU);   // parameters[7]
        m_parameters.push_back(m_dt/LIF_C);     // parameters[8]
}

void LIFNeuron::evolve()
{
        double t = GetGlobalTime();

        if ((t - m_tPrevSpike) <= LIF_TARP) {
                VM = LIF_ER;
        }
        else {
                double Iinj = LIF_IEXT;
                int i, nInputs = m_inputs.size();
                for (i=0; i<nInputs; i++)
                        Iinj += m_inputs[i];
                //VM = VM + m_dt/LIF_TAU * (LIF_E0 - VM) + m_dt/LIF_C * LIF_IEXT;
                VM = VM + m_parameters[7] * (LIF_E0 - VM) + m_parameters[8] * Iinj;
        }
        if(VM > LIF_VTH) {
#ifdef LIF_ARTIFICIAL_SPIKE
                /* an ``artificial'' spike is added when the membrane potential reaches threshold */
                VM = -LIF_VTH;
#else
                /* no ``artificial'' spike is added when the membrane potential reaches threshold */
                VM = LIF_E0;
#endif
                m_tPrevSpike = t;
                emitSpike();
        }
}

#ifdef HAVE_LIBCOMEDI

RealNeuron::RealNeuron(const char *kernelFile,
                       const char *deviceFile,
                       uint inputSubdevice, uint outputSubdevice,
                       uint readChannel, uint writeChannel,
                       double inputConversionFactor, double outputConversionFactor,
                       double spikeThreshold)
        : m_aec(kernelFile),
          m_analogInput(deviceFile, inputSubdevice, readChannel, inputConversionFactor),
          m_analogOutput(deviceFile, inputSubdevice, readChannel, inputConversionFactor)
{
        m_state.push_back(-65.);        // m_state[1] -> previous membrane voltage (for spike detection)
        m_parameters.push_back(spikeThreshold);
        m_parameters.push_back(spikeDelay);
}


RealNeuron::RealNeuron(const double *AECKernel, size_t kernelSize,
                       const char *deviceFile,
                       uint inputSubdevice, uint outputSubdevice,
                       uint readChannel, uint writeChannel,
                       double inputConversionFactor, double outputConversionFactor,
                       double spikeThreshold)
        : m_aec(AECKernel, kernelSize),
          m_analogInput(deviceFile, inputSubdevice, readChannel, inputConversionFactor),
          m_analogOutput(deviceFile, inputSubdevice, readChannel, inputConversionFactor)
{
        m_state.push_back(-65.);        // m_state[1] -> previous membrane voltage (for spike detection)
        m_parameters.push_back(spikeThreshold);
        m_parameters.push_back(spikeDelay);
}

RealNeuron::~RealNeuron()
{}

void RealNeuron::evolve()
{
        VM = aec.compensate( m_analogInput.read() );
        if (VM >= RN_SPIKE_THRESH && RN_VM_PREV < RN_SPIKE_THRESH)
                emitSpike();
        RN_VM_PREV = VM;
}

#endif // HAVE_LIBCOMEDI

} // namespace neurons

} // namespace dynclamp

