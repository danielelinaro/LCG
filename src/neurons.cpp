#include "neurons.h"
#include "events.h"
#include "thread_safe_queue.h"
#include "utils.h"
#include "engine.h"

dynclamp::Entity* LIFNeuronFactory(dictionary& args)
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

dynclamp::Entity* ConductanceBasedNeuronFactory(dictionary& args)
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

dynclamp::Entity* RealNeuronFactory(dictionary& args)
{
        uint inputSubdevice, outputSubdevice, readChannel, writeChannel, inputRange, reference, delaySteps, id;
        std::string kernelFile, deviceFile, inputRangeStr, referenceStr;
        double inputConversionFactor, outputConversionFactor, spikeThreshold, V0;
        bool adaptiveThreshold;

        id = dynclamp::GetIdFromDictionary(args);

        if ( ! dynclamp::CheckAndExtractValue(args, "deviceFile", deviceFile) ||
             ! dynclamp::CheckAndExtractUnsignedInteger(args, "inputSubdevice", &inputSubdevice) ||
             ! dynclamp::CheckAndExtractUnsignedInteger(args, "outputSubdevice", &outputSubdevice) ||
             ! dynclamp::CheckAndExtractUnsignedInteger(args, "readChannel", &readChannel) ||
             ! dynclamp::CheckAndExtractUnsignedInteger(args, "writeChannel", &writeChannel) ||
             ! dynclamp::CheckAndExtractDouble(args, "inputConversionFactor", &inputConversionFactor) ||
             ! dynclamp::CheckAndExtractDouble(args, "outputConversionFactor", &outputConversionFactor) ||
             ! dynclamp::CheckAndExtractUnsignedInteger(args, "delay", &delaySteps) ||
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

        if (! dynclamp::CheckAndExtractBool(args, "adaptiveThreshold", &adaptiveThreshold))
                adaptiveThreshold = false;

        return new dynclamp::neurons::RealNeuron(spikeThreshold, V0,
                                                 deviceFile.c_str(),
                                                 inputSubdevice, outputSubdevice, readChannel, writeChannel,
                                                 inputConversionFactor, outputConversionFactor,
                                                 inputRange, reference, delaySteps, adaptiveThreshold,
                                                 (kernelFile.compare("") == 0 ? NULL : kernelFile.c_str()), id);
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
                     uint id)
        : Neuron(E0, id)
{
        double dt = GetGlobalDt();
        m_parameters.push_back(C);
        m_parameters.push_back(tau);
        m_parameters.push_back(tarp);
        m_parameters.push_back(Er);
        m_parameters.push_back(E0);
        m_parameters.push_back(Vth);
        m_parameters.push_back(Iext);
        m_parameters.push_back(-1.0/LIF_TAU); // parameters[7] -> lambda
        m_parameters.push_back(LIF_TAU/LIF_C);// parameters[8] -> Rl
        m_parametersNames.push_back("C");
        m_parametersNames.push_back("tau");
        m_parametersNames.push_back("tarp");
        m_parametersNames.push_back("Er");
        m_parametersNames.push_back("E0");
        m_parametersNames.push_back("Vth");
        m_parametersNames.push_back("Iext");
        m_parametersNames.push_back("lambda");
        m_parametersNames.push_back("Rleak");
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

ConductanceBasedNeuron::ConductanceBasedNeuron(double C, double gl, double El, double Iext,
                                               double area, double spikeThreshold, double V0,
                                               uint id)
        : Neuron(V0, id)
{
        m_state.push_back(V0);                  // m_state[1] -> previous membrane voltage (for spike detection)

        m_parameters.push_back(C);              // m_parameters[0] -> capacitance
        m_parameters.push_back(gl);             // m_parameters[1] -> leak conductance
        m_parameters.push_back(El);             // m_parameters[2] -> leak reversal potential
        m_parameters.push_back(Iext);           // m_parameters[3] -> externally applied current
        m_parameters.push_back(area);           // m_parameters[4] -> area
        m_parameters.push_back(spikeThreshold); // m_parameters[5] -> spike threshold
        m_parameters.push_back(gl*10*area);     // m_parameters[6] -> leak conductance (in nS)
        m_parameters.push_back(GetGlobalDt() / (C*1e-5*area));   // m_parameters[7] -> coefficient
        m_parametersNames.push_back("C");
        m_parametersNames.push_back("gl");
        m_parametersNames.push_back("El");
        m_parametersNames.push_back("Iext");
        m_parametersNames.push_back("area");
        m_parametersNames.push_back("spikeThreshold");
        m_parametersNames.push_back("gl_in_nS");
        m_parametersNames.push_back("coeff");
        setName("ConductanceBasedNeuron");
        setUnits("mV");
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

RealNeuron::RealNeuron(double spikeThreshold, double V0,
                       const char *deviceFile,
                       uint inputSubdevice, uint outputSubdevice,
                       uint readChannel, uint writeChannel,
                       double inputConversionFactor, double outputConversionFactor,
                       uint inputRange, uint reference, uint voltageDelaySteps, bool adaptiveThreshold,
                       const char *kernelFile, uint id)
        : Neuron(V0, id), m_aec(kernelFile),
          m_input(deviceFile, inputSubdevice, readChannel, inputConversionFactor, inputRange, reference),
          m_output(deviceFile, outputSubdevice, writeChannel, outputConversionFactor, reference),
          m_delaySteps(voltageDelaySteps), m_VrDelay(NULL), m_adaptiveThreshold(adaptiveThreshold)
{
        m_state.push_back(V0);        // m_state[1] -> previous membrane voltage (for spike detection)
        m_parameters.push_back(spikeThreshold);
        m_parameters.push_back((double) m_delaySteps);
        m_parametersNames.push_back("spikeThreshold");
        m_parametersNames.push_back("delaySteps");
        setName("RealNeuron");
        setUnits("mV");
}

RealNeuron::RealNeuron(double spikeThreshold, double V0,
                       const char *deviceFile,
                       uint inputSubdevice, uint outputSubdevice,
                       uint readChannel, uint writeChannel,
                       double inputConversionFactor, double outputConversionFactor,
                       uint inputRange, uint reference, uint voltageDelaySteps,
                       const double *AECKernel, size_t kernelSize,
                       bool adaptiveThreshold, uint id)
        : Neuron(V0, id), m_aec(AECKernel, kernelSize),
          m_input(deviceFile, inputSubdevice, readChannel, inputConversionFactor, inputRange, reference),
          m_output(deviceFile, outputSubdevice, writeChannel, outputConversionFactor, reference),
          m_delaySteps(voltageDelaySteps), m_VrDelay(NULL), m_adaptiveThreshold(adaptiveThreshold)
{
        m_state.push_back(V0);        // m_state[1] -> previous membrane voltage (for spike detection)
        m_parameters.push_back(spikeThreshold);
        m_parameters.push_back((double) m_delaySteps);
        m_parametersNames.push_back("spikeThreshold");
        m_parametersNames.push_back("delaySteps");
        setName("RealNeuron");
        setUnits("mV");
}

RealNeuron::~RealNeuron()
{
        if (m_VrDelay != NULL)
                delete m_VrDelay;
}

bool RealNeuron::initialise()
{
        if (! Neuron::initialise()  ||
            ! m_input.initialise()  ||
            ! m_output.initialise() ||
            ! m_aec.initialise())
                return false;

        if (m_VrDelay != NULL)
                delete m_VrDelay;

        if (m_delaySteps > 0)
                m_VrDelay = new double[(int) m_delaySteps];

        double Vr = m_input.read();
        for (int i=0; i<m_delaySteps; i++)
                m_VrDelay[i] = Vr;
        VM = m_aec.compensate(m_VrDelay[0]);

        m_aec.pushBack(0.0);

        m_Vth = RN_SPIKE_THRESH;
        m_Vmin = -80;
        m_Vmax = RN_SPIKE_THRESH + 20;

        return true;
}
        
uint RealNeuron::voltageDelaySteps() const
{
        return m_delaySteps;
}

void RealNeuron::evolve()
{
        int i;

        /*** VOLTAGE ***/

        // read current value of the membrane potential
        double Vr = m_input.read();
        // set previous value of the membrane potential
        RN_VM_PREV = VM;
        // compensate the recorded voltage
        if (m_delaySteps == 0) {
                VM = m_aec.compensate(Vr);
        }
        else {
                VM = m_aec.compensate(m_VrDelay[0]);
                for (i=0; i<m_delaySteps-1; i++)
                        m_VrDelay[i] = m_VrDelay[i+1];
                m_VrDelay[m_delaySteps-1] = Vr;
        }

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

        /*** CURRENT ***/

        double Iinj = 0.0;
        size_t nInputs = m_inputs.size();
        for (i=0; i<nInputs; i++)
                Iinj += m_inputs[i];
        // inject the total input current into the neuron
        m_output.write(Iinj);
        // store the injected current into the buffer of the AEC
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

