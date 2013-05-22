
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
        self.add_parameter('compress', compress)
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
        
