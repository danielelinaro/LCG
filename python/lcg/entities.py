
#
# This file contains definitions of all the entities, so that they can be used
# with the class that writes configuration files.
#
# Author: Daniele Linaro - May 2013
#

from config_writer import XMLEntry

class Entity (XMLEntry):
    def __init__(self, name, id, connections):
        super(Entity,self).__init__('Entity',name,id,connections)

class H5Recorder (Entity):
    def __init__(self, id, connections, compress=True, filename=None):
        super(H5Recorder,self).__init__('H5Recorder', id, connections)
        if compress:
            self.add_parameter('compress', compress)
        if not filename is None:
            self.add_parameter('filename', filename)

class TriggeredH5Recorder (Entity):
    def __init__(self, id, connections, before, after, compress=True, filename=None):
        super(TriggeredH5Recorder,self).__init__('TriggeredH5Recorder', id, connections)
        self.add_parameter('before', before)
        self.add_parameter('after', after)
        if compress:
            self.add_parameter('compress', compress)
        if not filename is None:
            self.add_parameter('filename', filename)

class ASCIIRecorder (Entity):
    def __init__(self, id, connections, compress=True, filename=''):
        super(ASCIIRecorder,self).__init__('ASCIIRecorder', id, connections)
        if len(filename) > 0:
            self.add_parameter('filename', filename)

class Waveform (Entity):
    def __init__(self, id, connections, filename, units, triggered=False):
        super(Waveform,self).__init__('Waveform', id, connections)
        self.add_parameter('filename', filename)
        self.add_parameter('units', units)
        self.add_parameter('triggered', triggered)

class OU (Entity):
    def __init__(self, id, connections, mean, stddev, tau, ic, units, interval, seed=0):
        Entity.__init__(self, 'OU', id, connections)
        self.add_parameter('mean', mean)
        self.add_parameter('stddev', stddev)
        self.add_parameter('tau', tau)
        self.add_parameter('initialCondition', ic)
        self.add_parameter('units', units)
        self.add_parameter('interval', '%g,%g' % (interval[0],interval[1]))
        self.add_parameter('seed', seed)

class OUNonStationary (Entity):
    def __init__(self, id, connections, tau, ic, units, interval, seed=0):
        Entity.__init__(self, 'OUNonStationary', id, connections)
        self.add_parameter('tau', tau)
        self.add_parameter('initialCondition', ic)
        self.add_parameter('units', units)
        self.add_parameter('interval', '%g,%g' % (interval[0],interval[1]))
        self.add_parameter('seed', seed)

class ConductanceStimulus (Entity):
    def __init__(self, id, connections, E):
        super(ConductanceStimulus,self).__init__('ConductanceStimulus', id, connections)
        self.add_parameter('E', E)

class NMDAConductanceStimulus (Entity):
    def __init__(self, id, connections, E, K1, K2):
        super(NMDAConductanceStimulus,self).__init__('NMDAConductanceStimulus', id, connections)
        self.add_parameter('E', E)
        self.add_parameter('K1', K1)
        self.add_parameter('K2', K2)

class LIFNeuron (Entity):
    def __init__(self, id, connections, C, tau, tarp, Er, E0, Vth, Iext,
                 holdLastValue=False, holdLastValueFilename='/tmp/LIF.lastValue'):
        super(LIFNeuron,self).__init__('LIFNeuron', id, connections)
        self.add_parameter('C', C)
        self.add_parameter('tau', tau)
        self.add_parameter('tarp', tarp)
        self.add_parameter('Er', Er)
        self.add_parameter('E0', E0)
        self.add_parameter('Vth', Vth)
        self.add_parameter('Iext', Iext)
        self.add_parameter('holdLastValue', holdLastValue)
        self.add_parameter('holdLastValueFilename', holdLastValueFilename)
        

class IzhikevicNeuron (Entity):
    def __init__(self, id, connections, a, b, c, d, Vspk, Iext):
        super(IzhikevicNeuron,self).__init__('IzhikevicNeuron', id, connections)
        self.add_parameter('a', a)
        self.add_parameter('b', b)
        self.add_parameter('c', c)
        self.add_parameter('d', d)
        self.add_parameter('Vspk', Vspk)
        self.add_parameter('Iext', Iext)

class ConductanceBasedNeuron (Entity):
    def __init__(self, id, connections, C, gl, El, Iext, area, spike_threshold, V0):
        super(ConductanceBasedNeuron,self).__init__('ConductanceBasedNeuron', id, connections)
        self.add_parameter('C', C)
        self.add_parameter('gl', gl)
        self.add_parameter('El', El)
        self.add_parameter('Iext', Iext)
        self.add_parameter('area', area)
        self.add_parameter('spikeThreshold', spike_threshold)
        self.add_parameter('V0', V0)

class RealNeuron (Entity):
    def __init__(self, id, connections, spikeThreshold, V0, deviceFile, inputSubdevice,
                 outputSubdevice, readChannel, writeChannel, inputConversionFactor,
                 outputConversionFactor, inputRange, reference, kernelFile = '',
                 holdLastValue = False, holdLastValueFilename=None, adaptiveThreshold = False):
        super(RealNeuron,self).__init__('RealNeuron', id, connections)
        self.add_parameter('spikeThreshold', spikeThreshold)
        self.add_parameter('V0', V0)
        self.add_parameter('deviceFile', deviceFile)
        self.add_parameter('inputSubdevice', inputSubdevice)
        self.add_parameter('outputSubdevice', outputSubdevice)
        self.add_parameter('readChannel', readChannel)
        self.add_parameter('writeChannel', writeChannel)
        self.add_parameter('inputConversionFactor', inputConversionFactor)
        self.add_parameter('outputConversionFactor', outputConversionFactor)
        self.add_parameter('inputRange', inputRange)
        self.add_parameter('reference', reference)
        if len(kernelFile) > 0:
            self.add_parameter('kernelFile', kernelFile)
        if holdLastValue:
            self.add_parameter('holdLastValue', 'true')
        if holdLastValue and not holdLastValueFilename is None:
            self.add_parameter('holdLastValueFilename', holdLastValueFilename)
        if adaptiveThreshold:
            self.add_parameter('adaptiveThreshold', 'true')

class AnalogInput (Entity):
    def __init__(self, id, connections, deviceFile, inputSubdevice, readChannel,
                 inputConversionFactor, range, aref, units):
        super(AnalogInput,self).__init__('AnalogInput', id, connections)
        self.add_parameter('deviceFile', deviceFile)
        self.add_parameter('inputSubdevice', inputSubdevice)
        self.add_parameter('readChannel', readChannel)
        self.add_parameter('inputConversionFactor', inputConversionFactor)
        self.add_parameter('range', range)
        self.add_parameter('aref', aref)
        self.add_parameter('units', units)
        
class AnalogOutput (Entity):
    def __init__(self, id, connections, deviceFile, outputSubdevice, writeChannel,
                 outputConversionFactor, aref, units, resetOutput):
        super(AnalogOutput,self).__init__('AnalogOutput', id, connections)
        self.add_parameter('deviceFile', deviceFile)
        self.add_parameter('outputSubdevice', outputSubdevice)
        self.add_parameter('writeChannel', writeChannel)
        self.add_parameter('outputConversionFactor', outputConversionFactor)
        self.add_parameter('aref', aref)
        self.add_parameter('units', units)
        self.add_parameter('resetOutput', resetOutput)

class DigitalInput (Entity):
    def __init__(self, id, connections, deviceFile, inputSubdevice, readChannel,
                 units, eventToSend):
        super(AnalogInput,self).__init__('AnalogInput', id, connections)
        self.add_parameter('deviceFile', deviceFile)
        self.add_parameter('inputSubdevice', inputSubdevice)
        self.add_parameter('readChannel', readChannel)
        self.add_parameter('units', units)
        self.add_parameter('eventToSend', eventToSend)
        
class Connection (Entity):
    def __init__(self, id, connections, delay):
        super(Connection,self).__init__('Connection', id, connections)
        self.add_parameter('delay', delay)

class SynapticConnection (Entity):
    def __init__(self, id, connections, delay, weight):
        super(SynapticConnection,self).__init__('SynapticConnection', id, connections)
        self.add_parameter('delay', delay)
        self.add_parameter('weight', weight)

class VariableDelayConnection (Entity):
    def __init__(self, id, connections):
        super(VariableDelayConnection,self).__init__('VariableDelayConnection', id, connections)

class Constant (Entity):
    def __init__(self, id, connections, value, units=''):
        super(Constant,self).__init__('Constant', id, connections)
        self.add_parameter('value', value)
        if len(units) > 0:
            self.add_parameter('units', units)

class ConstantFromFile (Entity):
    def __init__(self, id, connections, filename='', units=''):
        super(ConstantFromFile,self).__init__('ConstantFromFile', id, connections)
        if len(filename) > 0:
            self.add_parameter('filename', filename)
        if len(units) > 0:
            self.add_parameter('units', units)

class Converter (Entity):
    def __init__(self, id, connections, parameter_name):
        super(Converter,self).__init__('Converter', id, connections)
        self.add_parameter('parameterName', parameter_name)

class Delay (Entity):
    def __init__(self, id, connections, n_samples=1):
        super(Delay,self).__init__('Delay', id, connections)
        self.add_parameter('nSamples', n_samples)

class SobolDelay (Entity):
    def __init__(self, id, connections, start_sample=0,
                 minimum=0, maximum = -1):
        super(SobolDelay,self).__init__('SobolDelay', id, connections)
        self.add_parameter('startSample', start_sample)
        self.add_parameter('min', minimum)
        self.add_parameter('max', maximum)

class EventCounter (Entity):
    def __init__(self, id, connections, max_count, auto_reset=True,
                 event_to_count='SPIKE', event_to_send='TRIGGER'):
        super(EventCounter,self).__init__('EventCounter', id, connections)
        self.add_parameter('maxCount', max_count)
        self.add_parameter('autoReset', auto_reset)
        self.add_parameter('eventToCount', event_to_count)
        self.add_parameter('eventToSend', event_to_send)

class FrequencyEstimator (Entity):
    def __init__(self, id, connections, tau, initial_frequency=0.):
        super(FrequencyEstimator,self).__init__('FrequencyEstimator', id, connections)
        self.add_parameter('tau', tau)
        self.add_parameter('initialFrequency', initial_frequency)

class PeriodicPulse (Entity):
    def __init__(self, id, connections, frequency, duration, amplitude, units=''):
        super(PeriodicPulse,self).__init__('PeriodicPulse', id, connections)
        self.add_parameter('frequency', frequency)
        self.add_parameter('duration', duration)
        self.add_parameter('amplitude', amplitude)
        if len(units) > 0:
            self.add_parameter('units', units)

class PID (Entity):
    def __init__(self, id, connections, baseline, gp, gi, gd=0.0):
        super(PID,self).__init__('PID', id, connections)
        self.add_parameter('baselineCurrent', baseline)
        self.add_parameter('gp', gp)
        self.add_parameter('gi', gi)
        self.add_parameter('gd', gd)

class Poisson (Entity):
    def __init__(self, id, connections, rate, seed):
        super(Poisson,self).__init__('Poisson', id, connections)
        self.add_parameter('rate', rate)
        self.add_parameter('seed', seed)

class ProbabilityEstimator (Entity):
    def __init__(self, id, connections, tau, stimulation_frequency,
                 window, initial_probability):
        super(ProbabilityEstimator,self).__init__('ProbabilityEstimator', id, connections)
        self.add_parameter('tau', tau)
        self.add_parameter('stimulationFrequency', stimulation_frequency)
        self.add_parameter('window', window)
        self.add_parameter('initialProbability', initial_probability)

class PeriodicTrigger (Entity):
    def __init__(self, id, connections, frequency, delay=0, tend=None):
        super(PeriodicTrigger,self).__init__('PeriodicTrigger', id, connections)
        self.add_parameter('frequency', frequency)
        if delay > 0: 
            self.add_parameter('delay', delay)
        if not tend is None:
            self.add_parameter('tend', tend)

class Synapse (Entity):
    def __init__(self, name, id, connections, E):
        super(Synapse,self).__init__('Synapse', id, connections)
        self.add_parameter('E', E)

class ExponentialSynapse (Synapse):
    def __init__(self, id, connections, E, tau):
        super(ExponentialSynapse,self).__init__('ExponentialSynapse', id, connections, E)
        self.add_parameter('tau', tau)

class Exp2Synapse (Synapse):
    def __init__(self, id, connections, E, tau_rise, tau_decay):
        super(Exp2Synapse,self).__init__('Exp2Synapse', id, connections, E)
        self.add_parameter('tauRise', tau_rise)
        self.add_parameter('tauDecay', tau_decay)

class TMGSynapse (Synapse):
    def __init__(self, id, connections, E, U, tau1, tau_rec, tau_facil):
        super(TMGSynapse,self).__init__('TMGSynapse', id, connections, E)
        self.add_parameter('U', U)
        self.add_parameter('tau1', tau1)
        self.add_parameter('tau_rec', tau_rec)
        self.add_parameter('tau_facil', tau_facil)

class IonicCurrent (Entity):
    def __init__(self, name, id, connections, area, gbar, E):
        super(IonicCurrent,self).__init__('IonicCurrent', id, connections)
        self.add_parameter('area', area)
        self.add_parameter('gbar', gbar)
        self.add_parameter('E', E)

class HHSodium (IonicCurrent):
    def __init__(self, id, connections, area, gbar=0.12, E=50.):
        super(HHSodium,self).__init__('HHSodium', id, connections, area, gbar, E)

class HHPotassium (IonicCurrent):
    def __init__(self, id, connections, area, gbar=0.036, E=-77.):
        super(HHPotassium,self).__init__('HHPotassium', id, connections, area, gbar, E)

class HH2Sodium (IonicCurrent):
    def __init__(self, id, connections, area, gbar=0.12, E=50.,
                 vtraub=-63., temperature=36):
        super(HH2Sodium,self).__init__('HH2Sodium', id, connections, area, gbar, E)
        self.add_parameter('vtraub', vtraub)
        self.add_parameter('temperature', temperature)

class HH2Potassium (IonicCurrent):
    def __init__(self, id, connections, area, gbar=0.005, E=-90.,
                 vtraub=-63., temperature=36):
        super(HH2Potassium,self).__init__('HH2Potassium', id, connections, area, gbar, E)
        self.add_parameter('vtraub', vtraub)
        self.add_parameter('temperature', temperature)

class MCurrent (IonicCurrent):
    def __init__(self, id, connections, area, gbar=0.005, E=-90.,
                 tau_max=1000., temperature=36):
        super(MCurrent,self).__init__('MCurrent', id, connections, area, gbar, E)
        self.add_parameter('taumax', tau_max)
        self.add_parameter('temperature', temperature)

class TCurrent (IonicCurrent):
    def __init__(self, id, connections, area, gbar=0.002, E=120.,
                 q10=3., shift=2., cao=2., caiInf=2.4e-4, taur=0.005,
                 depth=0.1, temperature=36.):
        super(TCurrent,self).__init__('TCurrent', id, connections, area, gbar, E)
        self.add_parameter('q10', q10)
        self.add_parameter('shift', shift)
        self.add_parameter('cao', cao)
        self.add_parameter('caiInf', caiInf)
        self.add_parameter('taur', taur)
        self.add_parameter('depth', depth)
        self.add_parameter('temperature', temperature)

class NoisyIonicCurrent (IonicCurrent):
    def __init__(self, name, id, connections, area, gbar, E, gamma, seed=None):
        super(NoisyIonicCurrent,self).__init__('NoisyIonicCurrent', id, connections, area, gbar, E)
        self.add_parameter('gamma', gamma)
        if seed:
            self.add_parameter('seed', seed)

class HHSodiumCN (NoisyIonicCurrent):
    def __init__(self, id, connections, area, gbar=0.12, E=50., gamma=10, seed=None):
        super(HHSodiumCN,self).__init__('HHSodiumCN', id, connections, area, gbar, E)

class HHPotassiumCN (NoisyIonicCurrent):
    def __init__(self, id, connections, area, gbar=0.036, E=-77, gamma=10, seed=None):
        super(HHPotassiumCN,self).__init__('HHPotassiumCN', id, connections, area, gbar, E)

