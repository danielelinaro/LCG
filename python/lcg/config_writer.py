import os
from lxml import etree
import lcg
import numpy as np

__all__ = ['XMLEntry','XMLConfigurationFile',
           'completeWithDefaultValues','writeIOConfigurationFile',
           'writeConductanceStimulusConfigurationFile',
           'writeIOExternalTriggerConfigurationFile']

class XMLEntry (object):
    def __init__(self, entry, name, id, connections):
        self._entry = entry.lower()
        self._name = name
        self._id = id
        if isinstance(connections, int):
            self._connections = tuple([connections])
        else:
            self._connections = connections
        self._parameters = {}

    def __str__(self):
        s = '<' + self._entry + '>\n'
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
        s += '</' + self._entry + '>\n'
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


class XMLConfigurationFile (object):
    """
    Class for creating configuration files.
    This class is a part of lcg.
    """
    def __init__(self, sampling_rate, trial_duration, output_filename=None, 
                 trigger = {'device': None,
                            'subdevice': None,
                            'channel': None}):
        '''
        Initializes an XMLConfigurationFile.
        '''
        self._xml_root = etree.Element('lcg')
        self._xml_entities = None
        self._xml_streams = None
        self._xml_simulation = etree.SubElement(self._xml_root,'simulation')
        self._add_elements(self._xml_simulation,{'rate':sampling_rate,'tend':trial_duration,
                                                 'outfile':output_filename})
        # Add trigger if present
        if ((not trigger['device'] is None) and 
            (not trigger['subdevice'] is None) and
            (not trigger['channel'] is None)):
            self._xml_simulation_trigger = etree.SubElement(self._xml_simulation,'trigger')
            self._add_elements(self._xml_simulation_trigger,{'device':trigger['device'],
                                                             'subdevice':trigger['subdevice'],
                                                             'channel':trigger['channel']})
        self._entities = []
        self._streams = []

    def _add_elements(self,group,parlist):
        '''
        Appends an XML Element to an etree.Element
        '''
        for a,o in parlist.iteritems():
            if not o is None:
                tmp = etree.SubElement(group, a)
                tmp.text = str(o)

    def _add_parameters(self,group,parlist):
        '''
        Appends dict of parameters to the SubElement "parameters" of an etree.Element
        '''
        parameters = etree.SubElement(group,"parameters")
        self._add_elements(parameters,parlist)

    def _add_connections(self,entity,connections):
        '''
        Appends list of numbers to the SubElement "connections" of an etree.Element
        '''
        if len(connections):
            connection = entity.find('connections')
            if connection is None:
                connection = etree.SubElement(entity, 'connections')
            if not connection.text is None:
                previous_conn = [int(ii) for ii in connection.text.split(',')]
                connections = np.unique(np.hstack([previous_conn, connections]).flatten())
            connection.text = ",".join(map(str,connections))

    def add_entity(self,entity):
        '''
        Adds an entity to the configuration file.
        '''
        if self._xml_entities is None:
            self._xml_entities = etree.SubElement(self._xml_root,'entities')
        self._entities.append(etree.SubElement(self._xml_entities,'entity'))
        self._add_elements(self._entities[-1], {'name': entity.name, 'id': entity.id})
        self._add_parameters(self._entities[-1], entity.parameters)
        self._add_connections(self._entities[-1], entity.connections)

    def add_stream(self,stream):
        '''
        Adds a stream to the configuration file.
        '''
        if self._xml_streams is None:
            self._xml_streams = etree.SubElement(self._xml_root,'streams')
        self._streams.append(etree.SubElement(self._xml_streams,'stream'))
        self._add_elements(self._streams[-1], {'name': stream.name, 'id': stream.id})
        self._add_parameters(self._streams[-1], stream.parameters)
        self._add_connections(self._streams[-1], stream.connections)
    
    def set_tend(self, tend):
        tmp = self._xml_simulation[1]
        tmp.text = str(tend)
        
    def write(self,filename):
        '''
        Saves the file.
        '''
        etree.ElementTree(self._xml_root).write(filename, pretty_print=True)

def completeWithDefaultValues(opt):
    if opt['type'] != 'input' and opt['type'] != 'output':
        print('Unknown channel type [%s].' % opt['type'])
    else:
        if 'mode' not in opt:
            opt['mode'] = 'CC'
        if 'device' not in opt:
            opt['device'] = os.environ['COMEDI_DEVICE']
        if 'subdevice' not in opt:
            if opt['type'] == 'input':
                opt['subdevice'] = os.environ['AI_SUBDEVICE']
            else:
                opt['subdevice'] = os.environ['AO_SUBDEVICE']
        if 'channel' not in opt:
            if opt['type'] == 'input':
                opt['channel'] = os.environ['AI_CHANNEL']
            else:
                opt['channel'] = os.environ['AO_CHANNEL']
        if 'factor' not in opt:
            if opt['type'] == 'input':
                opt['factor'] = os.environ['AI_CONVERSION_FACTOR_' + opt['mode']]
            else:
                opt['factor'] = os.environ['AO_CONVERSION_FACTOR_' + opt['mode']]
        if opt['type'] == 'input' and 'range' not in opt:
            opt['range'] = os.environ['RANGE']
        if 'reference' not in opt:
            opt['reference'] = os.environ['GROUND_REFERENCE']
        if 'units' not in opt:
            if opt['type'] == 'input':
                opt['units'] = os.environ['AI_UNITS_' + opt['mode']]
            else:
                opt['units'] = os.environ['AO_UNITS_' + opt['mode']]
        if opt['type'] == 'output':
            if 'offset' not in opt:
                opt['offset'] = 0.
            if 'resetOutput' not in opt:
                opt['resetOutput'] = True
    return opt

def writeIOConfigurationFile(config_file, sampling_rate, duration,
                             channels, realtime=True, output_filename=None,
                             model=None):
    config = lcg.XMLConfigurationFile(sampling_rate,
                                      duration,
                                      output_filename)
    ID = 0
    if realtime:
        config.add_entity(lcg.entities.H5Recorder(id=ID, 
                                                  filename=output_filename,
                                                  connections=()))
        ID += 1
    for chan in channels:
        try:
            chan = completeWithDefaultValues(chan)
        except KeyError:
            print('Each channel must contain a "type" key.')
            return False
        if model is None:
            if realtime:
                if chan['type'] == 'input':
                    config.add_entity(lcg.entities.AnalogInput(
                            id=ID, connections=(0), 
                            deviceFile=chan['device'],
                            inputSubdevice=chan['subdevice'], 
                            readChannel=chan['channel'],
                            inputConversionFactor=chan['factor'],
                            range=chan['range'],
                            aref=chan['reference'],
                            units=chan['units']))
                else:
                    config.add_entity(lcg.entities.AnalogOutput(
                            id=ID, connections=(),
                            deviceFile=chan['device'],
                            outputSubdevice=chan['subdevice'], 
                            writeChannel=chan['channel'],
                            outputConversionFactor=chan['factor'],
                            aref=chan['reference'], units=chan['units'], 
                            resetOutput=chan['resetOutput']))
                    config.add_entity(lcg.entities.Waveform(
                            id=ID+1,
                            connections=(0,ID), 
                            filename=chan['stimfile'], 
                            units=chan['units']))
                    ID += 1
            else:
                if chan['type'] == 'input':
                    config.add_stream(lcg.streams.InputChannel(
                            id=ID, connections=(), device=chan['device'],
                            subdevice=chan['subdevice'], 
                            channel=chan['channel'],
                            conversionFactor=chan['factor'],
                            range=chan['range'],
                            reference=chan['reference'],
                            units=chan['units'],
                            samplingRate=sampling_rate))
                else:
                    config.add_stream(lcg.streams.OutputChannel(
                            id=ID, connections=(), 
                            device=chan['device'],
                            subdevice=chan['subdevice'],
                            channel=chan['channel'],
                            conversionFactor=chan['factor'],
                            reference=chan['reference'],
                            units=chan['units'],
                            stimulusFile=chan['stimfile'],
                            samplingRate=sampling_rate,
                            offset=chan['offset'], 
                            resetOutput=chan['resetOutput']))
        elif model == 'LIF':
            if chan['type'] == 'input':
                config.add_entity(lcg.entities.LIFNeuron(
                        ID, (0), C=0.08, tau=0.0075,
                        tarp=0.0014, Er=-65.2,
                        E0=-70, Vth=-50, Iext=0,
                        holdLastValue=False))
            elif chan['type'] == 'output':
                config.add_entity(lcg.entities.Waveform(
                        id=ID, connections=(0,ID-1), 
                        filename=chan['stimfile'], 
                        units=chan['units']))        
        ID += 1
    config.write(config_file)
    return True

def writeConductanceStimulusConfigurationFile(config_file, sampling_rate, duration, 
                                              channels, reversal,model=None):
    config = lcg.XMLConfigurationFile(sampling_rate, duration)
    ID = 0
    realtime = True # Conductance stimulus requires a realtime engine for the moment.
    if realtime:
        config.add_entity(lcg.entities.H5Recorder(id=ID, connections=()))
        ID += 1
    input_channels = []
    output_channels = []
    used_outputs = []
    used_outputs_ids = []
    for chan in channels:
        try:
            chan = completeWithDefaultValues(chan)
        except KeyError:
            print('Each channel must contain a "type" key.')
            return False
        if chan['type'] == 'input':
            input_channels.append(chan)
        else:
            output_channels.append(chan)
    for ii in range(len(reversal)):
        chan = input_channels[ii]
        outchan = output_channels[ii]
        if not outchan['channel'] in used_outputs: # To allow multiple conductances to the same neuron 
            config.add_entity(lcg.entities.RealNeuron(id=ID, connections=(0), spikeThreshold = -10, V0 = -65, deviceFile=chan['device'],
                                                      inputSubdevice=chan['subdevice'], outputSubdevice=outchan['subdevice'],
                                                      readChannel=chan['channel'], writeChannel=outchan['channel'],
                                                      inputConversionFactor=chan['factor'], outputConversionFactor=outchan['factor'],
                                                      inputRange=chan['range'], reference=chan['reference'], kernelFile='kernel.dat'))
            config.add_entity(lcg.entities.Waveform(id=ID+1, connections=(0,ID+2), filename=outchan['stimfile'], units='nS'))
            config.add_entity(lcg.entities.ConductanceStimulus(id=ID+2, connections=(0,ID),E = reversal[ii]))
            used_outputs.append(outchan['channel'])
            used_outputs_ids.append(ID)
            ID += 3
        else:
            # Channel already used
            neuron_id = used_outputs_ids[np.nonzero(np.array(used_outputs)==outchan['channel'])[0]]
            config.add_entity(lcg.entities.Waveform(id=ID+1, connections=(0,ID+2), filename=outchan['stimfile'], units='nS'))
            config.add_entity(lcg.entities.ConductanceStimulus(id=ID+2, connections=(0,neuron_id),E = reversal[ii]))
            ID += 2
    for ii in range(len(input_channels)):
        if ii > len(reversal):
            chan = input_channels[ii]
            config.add_entity(lcg.entities.AnalogInput(id=ID, connections=(0), deviceFile=chan['device'],
                                                       inputSubdevice=chan['subdevice'], readChannel=chan['channel'],
                                                       inputConversionFactor=chan['factor'], range=chan['range'],
                                                       aref=chan['reference'], units=chan['units']))
            ID += 1
    for ii in range(len(output_channels)):
        if ii > len(reversal):
            chan = output_channels[ii]
            config.add_entity(lcg.entities.AnalogOutput(id=ID, connections=(), deviceFile=chan['device'],
                                                        aref=chan['reference'], units=chan['units'], resetOutput=chan['resetOutput']))
            ID += 1
    config.write(config_file)
    return True

def writeIOExternalTriggerConfigurationFile(config_file, sampling_rate, duration,
                                      channels, output_filename=None,
                                      trigger = {'device':None,
                                                 'subdevice':None,
                                                 'channel':None,
                                                 'stopChannel':None},
                                      digitalChannels = {'device':None,
                                                         'subdevice':None,
                                                         'channels':None}):

    config = lcg.XMLConfigurationFile(sampling_rate,
                                      duration,
                                      output_filename,
                                      trigger=trigger)
    ID = 0
    config.add_entity(lcg.entities.H5Recorder(id=ID, 
                                              filename=output_filename,
                                              connections=()))
    ID += 1
    for chan in channels:
        try:
            chan = completeWithDefaultValues(chan)
        except KeyError:
            print('Each channel must contain a "type" key.')
            return False
        if chan['type'] == 'input':
            config.add_entity(lcg.entities.AnalogInput(
                    id=ID, connections=(0), 
                    deviceFile=chan['device'],
                    inputSubdevice=chan['subdevice'], 
                    readChannel=chan['channel'],
                    inputConversionFactor=chan['factor'],
                    range=chan['range'],
                    aref=chan['reference'],
                    units=chan['units']))
        else:
            config.add_entity(lcg.entities.AnalogOutput(
                    id=ID, connections=(),
                    deviceFile=chan['device'],
                    outputSubdevice=chan['subdevice'], 
                    writeChannel=chan['channel'],
                    outputConversionFactor=chan['factor'],
                    aref=chan['reference'], units=chan['units'], 
                    resetOutput=chan['resetOutput']))
            config.add_entity(lcg.entities.Waveform(
                    id=ID+1,
                    connections=(0,ID), 
                    filename=chan['stimfile'], 
                    units=chan['units']))
            ID += 1
    ID += 1
    if not digitalChannels['channels'] is None:
        for channel in digitalChannels['channels']:
            config.add_entity(lcg.entities.DigitalInput(
                    id=ID, connections=(0), 
                    deviceFile=digitalChannels['device'],
                    inputSubdevice=digitalChannels['subdevice'], 
                    readChannel=channel,
                    units='Boolean',
                    eventToSend=None))
            ID += 1
    if not trigger['stopChannel'] is None:
            config.add_entity(lcg.entities.DigitalInput(
                    id=ID, connections=(0), 
                    deviceFile=trigger['device'],
                    inputSubdevice=trigger['subdevice'], 
                    readChannel=channel,
                    units='Boolean',
                    eventToSend='STOPRUN'))

    config.write(config_file)
    return True
