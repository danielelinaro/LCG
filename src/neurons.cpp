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

void Neuron::emitSpike(double timeout) const
{
#ifdef __APPLE__
        eventsQueue.push_back(new SpikeEvent(this, timeout));
#else
        EnqueueEvent(new SpikeEvent(this, timeout));
#endif
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
        if ((m_t-m_tPrevSpike) <= LIF_TARP) {
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
                m_tPrevSpike = m_t;
                emitSpike(2e-3);
        }
}

} // namespace neurons

} // namespace dynclamp

