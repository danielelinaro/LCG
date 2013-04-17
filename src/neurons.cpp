#include "neurons.h"
#include "events.h"
#include "thread_safe_queue.h"
#include "utils.h"
#include "engine.h"
#include <stdio.h>

dynclamp::Entity* LIFNeuronFactory(string_dict& args)
{
        uint id;
        double C, tau, tarp, Er, E0, Vth, Iext;

        id = dynclamp::GetIdFromDictionary(args);

        if ( ! dynclamp::CheckAndExtractDouble(args, "C", &C) ||
             ! dynclamp::CheckAndExtractDouble(args, "tau", &tau) ||
             ! dynclamp::CheckAndExtractDouble(args, "tarp", &tarp) ||
             ! dynclamp::CheckAndExtractDouble(args, "Er", &Er) ||
             ! dynclamp::CheckAndExtractDouble(args, "E0", &E0) ||
             ! dynclamp::CheckAndExtractDouble(args, "Vth", &Vth) ||
             ! dynclamp::CheckAndExtractDouble(args, "Iext", &Iext)) {
                dynclamp::Logger(dynclamp::Critical, "Unable to build a LIF neuron.\n");
                return NULL;
        }

        return new dynclamp::neurons::LIFNeuron(C, tau, tarp, Er, E0, Vth, Iext, id);
}

dynclamp::Entity* IzhikevichNeuronFactory(string_dict& args)
{
        uint id;
        double a, b, c, d, Vspk, Iext;

        id = dynclamp::GetIdFromDictionary(args);

		if (! dynclamp::CheckAndExtractDouble(args, "a", &a))
                a = 0.02;
		if (! dynclamp::CheckAndExtractDouble(args, "b", &b))
                b = 0.2;
		if (! dynclamp::CheckAndExtractDouble(args, "c", &c))
                c = -65;
		if (! dynclamp::CheckAndExtractDouble(args, "d", &d))
                Vspk = 2;
		if (! dynclamp::CheckAndExtractDouble(args, "Vspk", &Vspk))
                Vspk = 30;
		if (! dynclamp::CheckAndExtractDouble(args, "Iext", &Iext))
                Iext = 0;

        return new dynclamp::neurons::IzhikevichNeuron(a, b, c, d, Vspk, Iext, id);
}

dynclamp::Entity* ConductanceBasedNeuronFactory(string_dict& args)
{
        uint id;
        double C, gl, El, Iext, area, spikeThreshold, V0;
        id = dynclamp::GetIdFromDictionary(args);
        if ( ! dynclamp::CheckAndExtractDouble(args, "C", &C) ||
             ! dynclamp::CheckAndExtractDouble(args, "gl", &gl) ||
             ! dynclamp::CheckAndExtractDouble(args, "El", &El) ||
             ! dynclamp::CheckAndExtractDouble(args, "Iext", &Iext) ||
             ! dynclamp::CheckAndExtractDouble(args, "area", &area) ||
             ! dynclamp::CheckAndExtractDouble(args, "spikeThreshold", &spikeThreshold) ||
             ! dynclamp::CheckAndExtractDouble(args, "V0", &V0)) {
                dynclamp::Logger(dynclamp::Critical, "Unable to build a conductance based neuron.\n");
                return NULL;
        }

        return new dynclamp::neurons::ConductanceBasedNeuron(C, gl, El, Iext, area, spikeThreshold, V0, id);
}

#ifdef HAVE_LIBCOMEDI

dynclamp::Entity* RealNeuronFactory(string_dict& args)
{
        uint inputSubdevice, outputSubdevice, readChannel, writeChannel, inputRange, reference, id;
        std::string kernelFile, deviceFile, inputRangeStr, referenceStr;
        double inputConversionFactor, outputConversionFactor, spikeThreshold, V0;
        bool holdLastValue, adaptiveThreshold;

        id = dynclamp::GetIdFromDictionary(args);

        if ( ! dynclamp::CheckAndExtractValue(args, "deviceFile", deviceFile) ||
             ! dynclamp::CheckAndExtractUnsignedInteger(args, "inputSubdevice", &inputSubdevice) ||
             ! dynclamp::CheckAndExtractUnsignedInteger(args, "outputSubdevice", &outputSubdevice) ||
             ! dynclamp::CheckAndExtractUnsignedInteger(args, "readChannel", &readChannel) ||
             ! dynclamp::CheckAndExtractUnsignedInteger(args, "writeChannel", &writeChannel) ||
             ! dynclamp::CheckAndExtractDouble(args, "inputConversionFactor", &inputConversionFactor) ||
             ! dynclamp::CheckAndExtractDouble(args, "outputConversionFactor", &outputConversionFactor) ||
             ! dynclamp::CheckAndExtractDouble(args, "spikeThreshold", &spikeThreshold) ||
             ! dynclamp::CheckAndExtractDouble(args, "V0", &V0)) {
                dynclamp::Logger(dynclamp::Critical, "Unable to build a real neuron.\n");
                return NULL;
        }


        if (! dynclamp::CheckAndExtractValue(args, "kernelFile", kernelFile))
                kernelFile = "";

        if (! dynclamp::CheckAndExtractValue(args, "inputRange", inputRangeStr)) {
                inputRange = PLUS_MINUS_TEN;
        }
        else {
                if (inputRangeStr.compare("PlusMinusTen") == 0 ||
                    inputRangeStr.compare("[-10,+10]") == 0 ||
                    inputRangeStr.compare("+-10") == 0) {
                        inputRange = PLUS_MINUS_TEN;
                }
                else if (inputRangeStr.compare("PlusMinusFive") == 0 ||
                         inputRangeStr.compare("[-5,+5]") == 0 ||
                         inputRangeStr.compare("+-5") == 0) {
                        inputRange = PLUS_MINUS_FIVE;
                }
                else if (inputRangeStr.compare("PlusMinusOne") == 0 ||
                         inputRangeStr.compare("[-1,+1]") == 0 ||
                         inputRangeStr.compare("+-1") == 0) {
                        inputRange = PLUS_MINUS_ONE;
                }
                else if (inputRangeStr.compare("PlusMinusZeroPointTwo") == 0 ||
                         inputRangeStr.compare("[-0.2,+0.2]") == 0 ||
                         inputRangeStr.compare("+-0.2") == 0) {
                        inputRange = PLUS_MINUS_ZERO_POINT_TWO;
                }
                else {
                        dynclamp::Logger(dynclamp::Critical, "Unknown input range: [%s].\n", inputRangeStr.c_str());
                        dynclamp::Logger(dynclamp::Critical, "Unable to build a real neuron.\n");
                        return NULL;
                }
        }

        if (! dynclamp::CheckAndExtractValue(args, "reference", referenceStr)) {
                reference = GRSE;
        }
        else {
                if (referenceStr.compare("GRSE") == 0) {
                        reference = GRSE;
                }
                else if (referenceStr.compare("NRSE") == 0) {
                        reference = NRSE;
                }
                else {
                        dynclamp::Logger(dynclamp::Critical, "Unknown reference mode: [%s].\n", referenceStr.c_str());
                        dynclamp::Logger(dynclamp::Critical, "Unable to build a real neuron.\n");
                        return NULL;
                }
        }

        if (! dynclamp::CheckAndExtractBool(args, "holdLastValue", &holdLastValue))
                holdLastValue = false;

        if (! dynclamp::CheckAndExtractBool(args, "adaptiveThreshold", &adaptiveThreshold))
                adaptiveThreshold = false;

        return new dynclamp::neurons::RealNeuron(spikeThreshold, V0,
                                                 deviceFile.c_str(),
                                                 inputSubdevice, outputSubdevice, readChannel, writeChannel,
                                                 inputConversionFactor, outputConversionFactor,
                                                 inputRange, reference,
                                                 (kernelFile.compare("") == 0 ? NULL : kernelFile.c_str()),
                                                 holdLastValue, adaptiveThreshold, id);
}

#endif

namespace dynclamp {

extern ThreadSafeQueue<Event*> eventsQueue;

namespace neurons {

Neuron::Neuron(double Vm0, uint id)
        : DynamicalEntity(id), m_Vm0(Vm0)
{
        m_state.push_back(Vm0); // m_state[0] -> membrane potential
        setName("Neuron");
        setUnits("mV");
}

bool Neuron::initialise()
{
        VM = m_Vm0;
        return true;
}

double Neuron::Vm() const
{
        return VM;
}

double Neuron::Vm0() const
{
        return m_Vm0;
}

double Neuron::output()
{
        return VM;
}

void Neuron::emitSpike() const
{
        emitEvent(new SpikeEvent(this));
}

LIFNeuron::LIFNeuron(double C, double tau, double tarp,
                     double Er, double E0, double Vth, double Iext,
                     uint id)
        : Neuron(E0, id)
{
        double dt = GetGlobalDt();
        LIF_C = C;
        LIF_TAU = tau;
        LIF_TARP = tarp;
        LIF_ER = Er;
        LIF_E0 = E0;
        LIF_VTH = Vth;
        LIF_IEXT = Iext;
        LIF_LAMBDA = -1.0 / LIF_TAU;
        LIF_RL = LIF_TAU / LIF_C;
        setName("LIFNeuron");
        setUnits("mV");
}

bool LIFNeuron::initialise()
{
        if (! Neuron::initialise())
                return false;
        m_tPrevSpike = -1000.0;
        return true;
}

void LIFNeuron::evolve()
{
        double t = GetGlobalTime();

        if ((t - m_tPrevSpike) <= LIF_TARP) {
                VM = LIF_ER;
        }
        else {
                double Iinj = LIF_IEXT, Vinf;
                int nInputs = m_inputs.size();
                for (int i=0; i<nInputs; i++)
                        Iinj += m_inputs[i];
                Vinf = LIF_RL*Iinj + LIF_E0;
                VM = Vinf - (Vinf - VM) * exp(LIF_LAMBDA * GetGlobalDt());
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

IzhikevichNeuron::IzhikevichNeuron(double a, double b, double c,
                     double d, double Vspk, double Iext,
                     uint id)
        : Neuron(c, id)
{
        m_state.push_back(b*VM);                  // m_state[1] -> u the membrane recovery variable
        IZH_A = a;
        IZH_B = b;
        IZH_C = c;
        IZH_D = d;
        IZH_VSPK = Vspk;
        IZH_IEXT = Iext;
        setName("IzhikevichNeuron");
        setUnits("mV");
}

bool IzhikevichNeuron::initialise()
{
        if (! Neuron::initialise())
                return false;
        return true;
}

void IzhikevichNeuron::evolve()
{
        double t = GetGlobalTime();
        double dt = GetGlobalDt()*1e3; //parameters for the model are in ms.
        double Iinj = IZH_IEXT, k1, k2, k3, k4, l1, l2, l3, l4,x,y;
        int nInputs = m_inputs.size();
        for (int i=0; i<nInputs; i++)
             Iinj += m_inputs[i];
		
        


		// Runge-Kutta 4
		k1 = (0.04 * VM * VM) + 5 * VM + 140 - IZH_U + Iinj;
		l1 = IZH_A * (IZH_B * VM - IZH_U);
		
		x = VM + dt * 0.5 * k1; 
		y = IZH_U + dt * 0.5 * l1;
		k2 = (0.04 * (x * x) + 5 * x + 140 - y + Iinj); 
		l2 = IZH_A * (IZH_B * x - y);
	
		x = VM + dt * 0.5 * k2; 
		y = IZH_U + dt * 0.5 * l2;
		k3 = (0.04 * (x * x) + 5 * x + 140 - y + Iinj); 
		l3 = IZH_A * (IZH_B * x - y);
	
		x = VM + dt * k3; 
		y = IZH_U + dt * l3;
		k4 = (0.04 * (x * x) + 5 * x + 140 - y + Iinj); 
		l4 = IZH_A * (IZH_B * x - y);
		
		VM = VM + dt * ONE_OVER_SIX * (k1+2*k2+2*k3+k4);
		IZH_U = IZH_U + dt * ONE_OVER_SIX * (l1+2*l2+2*l3+l4);
		
		if(VM >= IZH_VSPK) {
                VM = IZH_C;
				IZH_U += IZH_D;
                emitSpike();
        }
}
ConductanceBasedNeuron::ConductanceBasedNeuron(double C, double gl, double El, double Iext,
                                               double area, double spikeThreshold, double V0,
                                               uint id)
        : Neuron(V0, id)
{
        m_state.push_back(V0);                  // m_state[1] -> previous membrane voltage (for spike detection)

        CBN_C = C;
        CBN_GL = gl;
        CBN_EL = El;
        CBN_IEXT = Iext;
        CBN_AREA = area;
        CBN_SPIKE_THRESH = spikeThreshold;
        CBN_GL_NS = gl*10*area; // leak conductance in nS
        CBN_COEFF = GetGlobalDt() / (C*1e-5*area); // coefficient

        setName("ConductanceBasedNeuron");
        setUnits("mV");
}

void ConductanceBasedNeuron::evolve()
{
        double Iinj, Ileak;
	
	Ileak = CBN_GL_NS * (CBN_EL - VM);	// (pA)
	
	Iinj = 0.;
	for(uint i=0; i<m_inputs.size(); i++)
		Iinj += m_inputs[i];	        // (pA)

        CBN_VM_PREV = VM;
        VM = VM + CBN_COEFF * (CBN_IEXT + Ileak + Iinj);

        if (VM >= CBN_SPIKE_THRESH && CBN_VM_PREV < CBN_SPIKE_THRESH)
                emitSpike();
}

#ifdef HAVE_LIBCOMEDI

RealNeuron::RealNeuron(double spikeThreshold, double V0,
                       const char *deviceFile,
                       uint inputSubdevice, uint outputSubdevice,
                       uint readChannel, uint writeChannel,
                       double inputConversionFactor, double outputConversionFactor,
                       uint inputRange, uint reference, const char *kernelFile,
                       bool holdLastValue, bool adaptiveThreshold, uint id)
        : Neuron(V0, id), m_aec(kernelFile),
          m_input(deviceFile, inputSubdevice, readChannel, inputConversionFactor, inputRange, reference),
          m_output(deviceFile, outputSubdevice, writeChannel, outputConversionFactor, reference),
          m_holdLastValue(holdLastValue), m_adaptiveThreshold(adaptiveThreshold)
{
        m_state.push_back(V0);        // m_state[1] -> previous membrane voltage (for spike detection)
        RN_SPIKE_THRESH = spikeThreshold;
        setName("RealNeuron");
        setUnits("mV");
}

RealNeuron::RealNeuron(double spikeThreshold, double V0,
                       const char *deviceFile,
                       uint inputSubdevice, uint outputSubdevice,
                       uint readChannel, uint writeChannel,
                       double inputConversionFactor, double outputConversionFactor,
                       uint inputRange, uint reference,
                       const double *AECKernel, size_t kernelSize,
                       bool holdLastValue, bool adaptiveThreshold, uint id)
        : Neuron(V0, id), m_aec(AECKernel, kernelSize),
          m_input(deviceFile, inputSubdevice, readChannel, inputConversionFactor, inputRange, reference),
          m_output(deviceFile, outputSubdevice, writeChannel, outputConversionFactor, reference),
          m_holdLastValue(holdLastValue), m_adaptiveThreshold(adaptiveThreshold)
{
        m_state.push_back(V0);        // m_state[1] -> previous membrane voltage (for spike detection)
        RN_SPIKE_THRESH = spikeThreshold;
        setName("RealNeuron");
        setUnits("mV");
}

RealNeuron::~RealNeuron()
{
        terminate();
}

bool RealNeuron::initialise()
{
        if (! Neuron::initialise()  ||
            ! m_input.initialise()  ||
            ! m_output.initialise())
                return false;

        if (m_holdLastValue && fs::exists(LOGFILE)) {
                FILE *fid = fopen(LOGFILE, "r");
                if (fid == NULL) {
                        Logger(Important, "Unable to open [%s].\n", LOGFILE);
                        m_Iinj = 0;
                }
                else {
                        fscanf(fid, "%le", &m_Iinj);
                        Logger(Important, "Successfully read holding value (%lf pA) from [%s].\n", m_Iinj, LOGFILE);
                        fclose(fid);
                }
        }
        else {
                m_Iinj = 0.0;
        }

        double Vr = m_input.read();
        if (! m_aec.initialise(m_Iinj,Vr))
                return false;
        VM = m_aec.compensate(Vr);
        m_aec.pushBack(m_Iinj);

        /*
        if (!m_aec.hasKernel()) {
                VM = Vr;
        }
        else {
                VM = m_aec.compensate(Vr);
                m_aec.pushBack(m_Iinj);
                for (int i=0; i<m_delaySteps; i++)
                        m_VrDelay[i] = Vr;
        }
        */

        m_Vth = RN_SPIKE_THRESH;
        m_Vmin = -80;
        m_Vmax = RN_SPIKE_THRESH + 20;

        return true;
}
        
void RealNeuron::terminate()
{
        Neuron::terminate();
        if (!m_holdLastValue)
                m_output.write(m_Iinj = 0);
        FILE *fid = fopen(LOGFILE, "w");
        if (fid != NULL) {
                fprintf(fid, "%le", m_Iinj);
                fclose(fid);
                Logger(Debug, "Written holding value (%lf pA) to [%s].\n", m_Iinj, LOGFILE);
        }
}

void RealNeuron::evolve()
{
        // compute the total input current
        m_Iinj = 0.0;
        size_t nInputs = m_inputs.size();
        for (int i=0; i<nInputs; i++)
                m_Iinj += m_inputs[i];
#ifndef TRIM_ANALOG_OUTPUT
        /*** BE SAFE! ***/
        if (m_Iinj < -10000)
                m_Iinj = -10000;
#endif
        // read current value of the membrane potential
        double Vr = m_input.read();
        // inject the total input current into the neuron
        m_output.write(m_Iinj);
        // store the previous value of the membrane potential
        RN_VM_PREV = VM;
        // compensate the recorded voltage
        VM = m_aec.compensate(Vr);
        // store the injected current into the buffer of the AEC
        m_aec.pushBack(m_Iinj);

        /*** SPIKE DETECTION ***/
        if (VM >= m_Vth && RN_VM_PREV < m_Vth) {
                emitSpike();
                if (m_adaptiveThreshold) {
                        m_Vmin = VM;
                        m_Vmax = VM;
                }
        }
        if (m_adaptiveThreshold) {
                if (VM < m_Vmin) {
                        m_Vmin = VM;
                }
                else if (VM > m_Vmax) {
                        m_Vmax = VM;
                        m_Vth = m_Vmax - 0.15 * (m_Vmax - m_Vmin);
                }
        }
}

bool RealNeuron::hasMetadata(size_t *ndims) const
{
        //if (m_aec.hasKernel())
        *ndims = 1;
        return m_aec.hasKernel();
}

const double* RealNeuron::metadata(size_t *dims, char *label) const
{
        //if (m_aec.hasKernel()) {
        sprintf(label, "Electrode_Kernel");
        dims[0] = m_aec.kernelLength();
        return m_aec.kernel();
        //}
        //return NULL;
}

#endif // HAVE_LIBCOMEDI

} // namespace neurons

} // namespace dynclamp

