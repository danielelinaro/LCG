#include "neurons.h"
#include "events.h"
#include "thread_safe_queue.h"
#include "utils.h"

#ifdef DEBUG_REAL_NEURON
#include <unistd.h>
#include <fcntl.h>
#endif

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
        m_state.push_back(V0);                  // m_state[1] -> previous membrane voltage (for spike detection)

        m_parameters.push_back(C);              // m_parameters[0] -> capacitance
        m_parameters.push_back(gl);             // m_parameters[1] -> leak conductance
        m_parameters.push_back(El);             // m_parameters[2] -> leak reversal potential
        m_parameters.push_back(Iext);           // m_parameters[3] -> externally applied current
        m_parameters.push_back(area);           // m_parameters[4] -> area
        m_parameters.push_back(spikeThreshold); // m_parameters[5] -> spike threshold
        m_parameters.push_back(gl*10*area);     // m_parameters[6] -> leak conductance (in nS)
        m_parameters.push_back(m_dt / (C*1e-5*area));   // m_parameters[7] -> coefficient
}

void ConductanceBasedNeuron::evolve()
{
        double Iinj, Ileak;
	
	Ileak = CBN_GL * (CBN_EL - VM);		// (pA)
	
	Iinj = 0.;
	for(uint i=0; i<m_inputs.size(); i++)
		Iinj += m_inputs[i];	        // (pA)

        CBN_VM_PREV = VM;
        VM = VM + CBN_COEFF * (CBN_IEXT + Ileak + Iinj);

        if (VM >= CBN_SPIKE_THRESH && CBN_VM_PREV < CBN_SPIKE_THRESH)
                emitSpike();
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
#ifdef DEBUG_REAL_NEURON
        char fname[FILENAME_MAXLEN];
        MakeFilename(fname,".bin");
        m_fd = open(fname, O_WRONLY);
        m_bufpos = 0;
#endif
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
#ifdef DEBUG_REAL_NEURON
        close(m_fd);
#endif
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
        if (fabs(Iinj) > 2000) {
                Logger(Critical, "%e %e %e %e\n", GetGlobalTime(), VM, Vr, Iinj);
                Iinj = 2000 * fabs(Iinj)/Iinj;
        }
        m_output.write(Iinj);
        m_aec.pushBack(Iinj);
#ifdef DEBUG_REAL_NEURON
        m_buffer[m_bufpos] = Vr;
        m_buffer[m_bufpos+1] = VM;
        m_buffer[m_bufpos+2] = Iinj;
        m_bufpos = (m_bufpos+3) % RN_BUFLEN;
        if (m_bufpos == 0)
                write(m_fd, (const void *) m_buffer, RN_BUFLEN*sizeof(double));
#endif
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

