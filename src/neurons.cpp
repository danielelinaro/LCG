#include "neurons.h"
#include "events.h"
#include "thread_safe_queue.h"
#include "utils.h"

dynclamp::Entity* LIFNeuronFactory(dictionary& args)
{
        uint id;
        double C, tau, tarp, Er, E0, Vth, Iext, dt;

        dynclamp::GetIdAndDtFromDictionary(args, &id, &dt);

        if ( ! dynclamp::CheckAndExtractDouble(args, "C", &C) ||
             ! dynclamp::CheckAndExtractDouble(args, "tau", &tau) ||
             ! dynclamp::CheckAndExtractDouble(args, "tarp", &tarp) ||
             ! dynclamp::CheckAndExtractDouble(args, "Er", &Er) ||
             ! dynclamp::CheckAndExtractDouble(args, "E0", &E0) ||
             ! dynclamp::CheckAndExtractDouble(args, "Vth", &Vth) ||
             ! dynclamp::CheckAndExtractDouble(args, "Iext", &Iext))
                return NULL;

        return new dynclamp::neurons::LIFNeuron(C, tau, tarp, Er, E0, Vth, Iext, id, dt);
}

#ifdef HAVE_LIBCOMEDI

dynclamp::Entity* RealNeuronFactory(dictionary& args)
{
        uint inputSubdevice, outputSubdevice, readChannel, writeChannel, id;
        std::string kernelFile, deviceFile;
        double inputConversionFactor, outputConversionFactor, spikeThreshold, V0, dt;

        dynclamp::GetIdAndDtFromDictionary(args, &id, &dt);

        if ( ! dynclamp::CheckAndExtractValue(args, "kernelFile", kernelFile) ||
             ! dynclamp::CheckAndExtractValue(args, "deviceFile", deviceFile) ||
             ! dynclamp::CheckAndExtractUnsignedInteger(args, "inputSubdevice", &inputSubdevice) ||
             ! dynclamp::CheckAndExtractUnsignedInteger(args, "outputSubdevice", &outputSubdevice) ||
             ! dynclamp::CheckAndExtractUnsignedInteger(args, "readChannel", &readChannel) ||
             ! dynclamp::CheckAndExtractUnsignedInteger(args, "writeChannel", &writeChannel) ||
             ! dynclamp::CheckAndExtractDouble(args, "inputConversionFactor", &inputConversionFactor) ||
             ! dynclamp::CheckAndExtractDouble(args, "outputConversionFactor", &outputConversionFactor) ||
             ! dynclamp::CheckAndExtractDouble(args, "spikeThreshold", &spikeThreshold) ||
             ! dynclamp::CheckAndExtractDouble(args, "V0", &V0))
                return NULL;

        return new dynclamp::neurons::RealNeuron(kernelFile.c_str(), deviceFile.c_str(),
                                                 inputSubdevice, outputSubdevice, readChannel, writeChannel,
                                                 inputConversionFactor, outputConversionFactor,
                                                 spikeThreshold, V0, id, dt);
}

#endif

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

ConductanceBasedNeuron::ConductanceBasedNeuron(double C, double gl, double El, double Iext,
                                               double area, double spikeThreshold, double V0,
                                               uint id, double dt)
        : Neuron(V0, id, dt)
{
        m_parameters.push_back(C*1e-8*area);    // m_parameters[0] -> capacitance
        m_parameters.push_back(gl);             // m_parameters[1] -> leak conductance
        m_parameters.push_back(El);             // m_parameters[2] -> leak reversal potential
        m_parameters.push_back(Iext);           // m_parameters[3] -> externally applied current
        m_parameters.push_back(area);           // m_parameters[4] -> area
        m_parameters.push_back(spikeThreshold); // m_parameters[5] -> spike threshold
}

void ConductanceBasedNeuron::evolve()
{
        double Iion, Ileak;
	
	Ileak = CBN_GL * (CBN_EL - VM);		// (mA/cm^2)
	
	Iion = 0.;
	for(uint i=0; i<m_inputs.size(); i++) {
		Iion += m_inputs[i];	        // (pA)
	}
        Iion = Iion * 1e-9 / (CBN_AREA * 1e-8); // (mA/cm^2)

	//v = v + 1e-3 * (dt/(Cm*1e-6)) * (Iext*1e-9 - (Ileak+Iion)*(1e-8*area));
	VM = VM + 1e-6 * m_dt/CBN_C * (CBN_IEXT - 10*CBN_AREA*(Ileak+Iion));
        
}

#ifdef HAVE_LIBCOMEDI

RealNeuron::RealNeuron(const char *kernelFile,
                       const char *deviceFile,
                       uint inputSubdevice, uint outputSubdevice,
                       uint readChannel, uint writeChannel,
                       double inputConversionFactor, double outputConversionFactor, double spikeThreshold,
                       double V0, uint id, double dt)
        : Neuron(V0, id, dt), m_aec(kernelFile),
          m_input(deviceFile, inputSubdevice, readChannel, inputConversionFactor),
          m_output(deviceFile, outputSubdevice, writeChannel, outputConversionFactor)
{
        m_state.push_back(V0);        // m_state[1] -> previous membrane voltage (for spike detection)
        m_parameters.push_back(spikeThreshold);
}


RealNeuron::RealNeuron(const double *AECKernel, size_t kernelSize,
                       const char *deviceFile,
                       uint inputSubdevice, uint outputSubdevice,
                       uint readChannel, uint writeChannel,
                       double inputConversionFactor, double outputConversionFactor, double spikeThreshold,
                       double V0, uint id, double dt)
        : Neuron(V0, id, dt), m_aec(AECKernel, kernelSize),
          m_input(deviceFile, inputSubdevice, readChannel, inputConversionFactor),
          m_output(deviceFile, outputSubdevice, writeChannel, outputConversionFactor)
{
        m_state.push_back(V0);        // m_state[1] -> previous membrane voltage (for spike detection)
        m_parameters.push_back(spikeThreshold);
}
        
void RealNeuron::evolve()
{
        // read current value of the membrane potential
        RN_VM_PREV = VM;
        double Vr = m_input.read();
        VM = m_aec.compensate( Vr );
        if (VM >= RN_SPIKE_THRESH && RN_VM_PREV < RN_SPIKE_THRESH)
                emitSpike();

        // inject the total input current into the neuron
        double Iinj = 0.0;
        size_t nInputs = m_inputs.size();
        for (uint i=0; i<nInputs; i++) {
                Iinj += m_inputs[i];
        }
        m_output.write(Iinj);
        m_aec.pushBack(Iinj);
}

bool RealNeuron::hasMetadata(size_t *ndims) const
{
        *ndims = 1;
        return true;
}

const double* RealNeuron::metadata(size_t *dims, char *label) const
{
        sprintf(label, "Electrode_Kernel");
        dims[0] = m_aec.kernelLength();
        return m_aec.kernel();
}

#endif // HAVE_LIBCOMEDI

} // namespace neurons

} // namespace dynclamp

