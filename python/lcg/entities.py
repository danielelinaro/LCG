
#
# This file contains definitions of all the entities, so that they can be used
# with the class that writes configuration files.
#
# Author: Daniele Linaro - May 2013
#

class Entity (object):
    def __init__(self, name, id, connections):
        self._name = name
        self._id = id
        if isinstance(connections, int):
            self._connections = tuple([connections])
        else:
            self._connections = connections
        self._parameters = {}

    def __str__(self):
        s = '<entity>\n'
        s += ('<name>'+self._name+'</name>\n')
        s += ('<id>'+str(self._id)+'</id>\n')
        s += '<parameters>\n'
        for k,v in self._parameters.iteritems():
            s += ('<'+k+'>'+str(v)+'</'+k+'>\n')
        s += '</parameters>\n'
        if len(self._connections) > 0:
            s += '<connections>'
            for i,c in enumerate(self._connections):
                s += str(c)
                if i < len(self._connections)-1:
                    s += ','
            s += '</connections>\n'
        s += '</entity>\n'
        return s

    def add_parameter(self, key, value):
        self._parameters[key] = value

    def parameter(self, key):
        return self._parameters[key]

    def parameter_names(self):
        return self._parameters.keys()
    
    def parameter_values(self):
        return self._parameters.values()

    def iter_parameters(self):
        for k,v in self._parameters.iteritems():
            yield k,v
    
    @property
    def name(self):
        return self._name

    @property
    def id(self):
        return self._id

    @property
    def parameters(self):
        return self._parameters

    @property
    def connections(self):
        return self._connections

    @property
    def parameters(self):
        return self._parameters

class H5Recorder (Entity):
    def __init__(self, id, connections, compress=True, filename=''):
        Entity.__init__(self, 'H5Recorder', id, connections)
        if compress:
            self.add_parameter('compress', compress)
        if len(filename) > 0:
            self.add_parameter('filename', filename)

class TriggeredH5Recorder (Entity):
    def __init__(self, id, connections, before, after, compress=True, filename=''):
        Entity.__init__(self, 'TriggeredH5Recorder', id, connections)
        self.add_parameter('before', before)
        self.add_parameter('after', after)
        if compress:
            self.add_parameter('compress', compress)
        if len(filename) > 0:
            self.add_parameter('filename', filename)

class ASCIIRecorder (Entity):
    def __init__(self, id, connections, compress=True, filename=''):
        Entity.__init__(self, 'ASCIIRecorder', id, connections)
        if len(filename) > 0:
            self.add_parameter('filename', filename)

class Waveform (Entity):
    def __init__(self, id, connections, filename, units, triggered=False):
        Entity.__init__(self, 'Waveform', id, connections)
        self.add_parameter('filename', filename)
        self.add_parameter('units', units)
        self.add_parameter('triggered', triggered)

class ConductanceStimulus (Entity):
    def __init__(self, id, connections, E):
        Entity.__init__(self, 'ConductanceStimulus', id, connections)
        self.add_parameter('E', E)

class LIFNeuron (Entity):
    def __init__(self, id, connections, C, tau, tarp, Er, E0, Vth, Iext):
        Entity.__init__(self, 'LIFNeuron', id, connections)
        self.add_parameter('C', C)
        self.add_parameter('tau', tau)
        self.add_parameter('tarp', tarp)
        self.add_parameter('Er', Er)
        self.add_parameter('E0', E0)
        self.add_parameter('Vth', Vth)
        self.add_parameter('Iext', Iext)

class IzhikevicNeuron (Entity):
    def __init__(self, id, connections, a, b, c, d, Vspk, Iext):
        Entity.__init__(self, 'IzhikevicNeuron', id, connections)
        self.add_parameter('a', a)
        self.add_parameter('b', b)
        self.add_parameter('c', c)
        self.add_parameter('d', d)
        self.add_parameter('Vspk', Vspk)
        self.add_parameter('Iext', Iext)

class ConductanceBasedNeuron (Entity):
    def __init__(self, id, connections, C, gl, El, Iext, area, spike_threshold, V0):
        Entity.__init__(self, 'ConductanceBasedNeuron', id, connections)
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
                 holdLastValue = False, adaptiveThreshold = False):
        Entity.__init__(self, 'RealNeuron', id, connections)
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
        if adaptiveThreshold:
            self.add_parameter('adaptiveThreshold', 'true')

class AnalogInput (Entity):
    def __init__(self, id, connections, deviceFile, inputSubdevice, readChannel,
                 inputConversionFactor, range, aref, units):
        Entity.__init__(self, 'AnalogInput', id, connections)
        self.add_parameter('deviceFile', deviceFile)
        self.add_parameter('inputSubdevice', inputSubdevice)
        self.add_parameter('readChannel', readChannel)
        self.add_parameter('inputConversionFactor', inputConversionFactor)
        self.add_parameter('range', range)
        self.add_parameter('aref', aref)
        self.add_parameter('units', units)
        
class AnalogOutput (Entity):
    def __init__(self, id, connections, deviceFile, outputSubdevice, writeChannel,
                 outputConversionFactor, aref, units):
        Entity.__init__(self, 'AnalogOutput', id, connections)
        self.add_parameter('deviceFile', deviceFile)
        self.add_parameter('outputSubdevice', outputSubdevice)
        self.add_parameter('writeChannel', writeChannel)
        self.add_parameter('outputConversionFactor', outputConversionFactor)
        self.add_parameter('aref', aref)
        self.add_parameter('units', units)
        
class Connection (Entity):
    def __init__(self, id, connections, delay):
        Entity.__init__(self, 'Connection', id, connections)
        self.add_parameter('delay', delay)

class SynapticConnection (Entity):
    def __init__(self, id, connections, delay, weight):
        Entity.__init__(self, 'SynapticConnection', id, connections)
        self.add_parameter('delay', delay)
        self.add_parameter('weight', weight)

class VariableDelayConnection (Entity):
    def __init__(self, id, connections):
        Entity.__init__(self, 'VariableDelayConnection', id, connections)

class Constant (Entity):
    def __init__(self, id, connections, value, units=''):
        Entity.__init__(self, 'Constant', id, connections)
        self.add_parameter('value', value)
        if len(units) > 0:
            self.add_parameter('units', units)

class ConstantFromFile (Entity):
    def __init__(self, id, connections, filename='', units=''):
        Entity.__init__(self, 'ConstantFromFile', id, connections)
        if len(filename) > 0:
            self.add_parameter('filename', filename)
        if len(units) > 0:
            self.add_parameter('units', units)

class Converter (Entity):
    def __init__(self, id, connections, parameter_name):
        Entity.__init__(self, 'Converter', id, connections)
        self.add_parameter('parameterName', parameter_name)

class Delay (Entity):
    def __init__(self, id, connections, n_samples=1):
        Entity.__init__(self, 'Delay', id, connections)
        self.add_parameter('nSamples', n_samples)

class EventCounter (Entity):
    def __init__(self, id, connections, max_count, auto_reset=True,
                 event_to_count='SPIKE', event_to_send='TRIGGER'):
        Entity.__init__(self, 'EventCounter', id, connections)
        self.add_parameter('maxCount', max_count)
        self.add_parameter('autoReset', auto_reset)
        self.add_parameter('eventToCount', event_to_count)
        self.add_parameter('eventToSend', event_to_send)

class FrequencyEstimator (Entity):
    def __init__(self, id, connections, tau, initial_frequency=0.):
        Entity.__init__(self, 'FrequencyEstimator', id, connections)
        self.add_parameter('tau', tau)
        self.add_parameter('initialFrequency', initial_frequency)

class PeriodicPulse (Entity):
    def __init__(self, id, connections, frequency, duration, amplitude, units=''):
        Entity.__init__(self, 'PeriodicPulse', id, connections)
        self.add_parameter('frequency', frequency)
        self.add_parameter('duration', duration)
        self.add_parameter('amplitude', amplitude)
        if len(units) > 0:
            self.add_parameter('units', units)

class PID (Entity):
    def __init__(self, id, connections, baseline, gp, gi, gd=0.0):
        Entity.__init__(self, 'PID', id, connections)
        self.add_parameter('baselineCurrent', baseline)
        self.add_parameter('gp', gp)
        self.add_parameter('gi', gi)
        self.add_parameter('gd', gd)

class Poisson (Entity):
    def __init__(self, id, connections, rate, seed):
        Entity.__init__(self, 'Poisson', id, connections)
        self.add_parameter('rate', rate)
        self.add_parameter('seed', seed)

class ProbabilityEstimator (Entity):
    def __init__(self, id, connections, tau, stimulation_frequency,
                 window, initial_probability):
        Entity.__init__(self, 'ProbabilityEstimator', id, connections)
        self.add_parameter('tau', tau)
        self.add_parameter('stimulationFrequency', stimulation_frequency)
        self.add_parameter('window', window)
        self.add_parameter('initialProbability', initial_probability)

class PeriodicTrigger (Entity):
    def __init__(self, id, connections, frequency):
        Entity.__init__(self, 'PeriodicTrigger', id, connections)
        self.add_parameter('frequency', frequency)

class Synapse (Entity):
    def __init__(self, name, id, connections, E):
        Entity.__init__(self, name, id, connections)
        self.add_parameter('E', E)

class ExponentialSynapse (Synapse):
    def __init__(self, id, connections, E, tau):
        Synapse.__init__(self, 'ExponentialSynapse', id, connections, E)
        self.add_parameter('tau', tau)

class Exp2Synapse (Synapse):
    def __init__(self, id, connections, E, tau_rise, tau_decay):
        Synapse.__init__(self, 'Exp2Synapse', id, connections, E)
        self.add_parameter('tauRise', tau_rise)
        self.add_parameter('tauDecay', tau_decay)

class TMGSynapse (Synapse):
    def __init__(self, id, connections, E, U, tau1, tau_rec, tau_facil):
        Synapse.__init__(self, 'TMGSynapse', id, connections, E)
        self.add_parameter('U', U)
        self.add_parameter('tau1', tau1)
        self.add_parameter('tau_rec', tau_rec)
        self.add_parameter('tau_facil', tau_facil)

class IonicCurrent (Entity):
    def __init__(self, name, id, connections, area, gbar, E):
        Entity.__init__(self, name, id, connections)
        self.add_parameter('area', area)
        self.add_parameter('gbar', gbar)
        self.add_parameter('E', E)

class HHSodium (IonicCurrent):
    def __init__(self, id, connections, area, gbar=0.12, E=50.):
        IonicCurrent.__init__(self, 'HHSodium', id, connections, area, gbar, E)

class HHPotassium (IonicCurrent):
    def __init__(self, id, connections, area, gbar=0.036, E=-77.):
        IonicCurrent.__init__(self, 'HHPotassium', id, connections, area, gbar, E)

class HH2Sodium (IonicCurrent):
    def __init__(self, id, connections, area, gbar=0.12, E=50.,
                 vtraub=-63., temperature=36):
        IonicCurrent.__init__(self, 'HH2Sodium', id, connections, area, gbar, E)
        self.add_parameter('vtraub', vtraub)
        self.add_parameter('temperature', temperature)

class HH2Potassium (IonicCurrent):
    def __init__(self, id, connections, area, gbar=0.005, E=-90.,
                 vtraub=-63., temperature=36):
        IonicCurrent.__init__(self, 'HH2Potassium', id, connections, area, gbar, E)
        self.add_parameter('vtraub', vtraub)
        self.add_parameter('temperature', temperature)

class MCurrent (IonicCurrent):
    def __init__(self, id, connections, area, gbar=0.005, E=-90.,
                 tau_max=1000., temperature=36):
        IonicCurrent.__init__(self, 'MCurrent', id, connections, area, gbar, E)
        self.add_parameter('taumax', tau_max)
        self.add_parameter('temperature', temperature)

class TCurrent (IonicCurrent):
    def __init__(self, id, connections, area, gbar=0.002, E=120.,
                 q10=3., shift=2., cao=2., caiInf=2.4e-4, taur=0.005,
                 depth=0.1, temperature=36.):
        IonicCurrent.__init__(self, 'TCurrent', id, connections, area, gbar, E)
        self.add_parameter('q10', q10)
        self.add_parameter('shift', shift)
        self.add_parameter('cao', cao)
        self.add_parameter('caiInf', caiInf)
        self.add_parameter('taur', taur)
        self.add_parameter('depth', depth)
        self.add_parameter('temperature', temperature)

class NoisyIonicCurrent (IonicCurrent):
    def __init__(self, name, id, connections, area, gbar, E, gamma, seed=None):
        IonicCurrent.__init__(self, name, id, connections, area, gbar, E)
        self.add_parameter('gamma', gamma)
        if seed:
            self.add_parameter('seed', seed)

class HHSodiumCN (NoisyIonicCurrent):
    def __init__(self, id, connections, area, gbar=0.12, E=50., gamma=10, seed=None):
        NoisyIonicCurrent.__init__(self, 'HHSodiumCN', id, connections, area, gbar, E, gamma, seed)

class HHPotassiumCN (NoisyIonicCurrent):
    def __init__(self, id, connections, area, gbar=0.036, E=-77, gamma=10, seed=None):
        NoisyIonicCurrent.__init__(self, 'HHPotassiumCN', id, connections, area, gbar, E, gamma, seed)
