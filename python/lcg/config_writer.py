
import os
from lxml import etree
import lcg

__all__ = ['XMLEntry','XMLConfigurationFile','completeWithDefaultValues','writeIOConfigurationFile']

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
    def __init__(self, sampling_rate, trial_duration):
        '''
        Initializes an XMLConfigurationFile.
        '''
        self._xml_root = etree.Element('lcg')
        self._xml_entities = None
        self._xml_streams = None
        self._xml_simulation = etree.SubElement(self._xml_root,'simulation')
        self._add_elements(self._xml_simulation,{'rate':sampling_rate,'tend':trial_duration})
        self._entities = []
        self._streams = []

    def _add_elements(self,group,parlist):
        '''
        Appends an XML Element to an etree.Element
        '''
        for a,o in parlist.iteritems():
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
                previous_conn = [str(ii) for ii in connection.text.split(',')]
                connections = np.unique(np.array([previous_conn, connections])).flatten()
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
        if 'conversionFactor' not in opt:
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
    return opt

def writeIOConfigurationFile(config_file, sampling_rate, duration, channels):
    config = lcg.XMLConfigurationFile(sampling_rate,duration)
    ID = 0
    for chan in channels:
        try:
            chan = completeWithDefaultValues(chan)
        except KeyError:
            print('Each channel must contain a "type" key.')
            return False
        if chan['type'] == 'input':
            config.add_stream(lcg.streams.InputChannel(id=ID, connections=(), device=chan['device'],
                                                       subdevice=chan['subdevice'], channel=chan['channel'],
                                                       conversionFactor=chan['factor'], range=chan['range'],
                                                       reference=chan['reference'], units=chan['units'],
                                                       samplingRate=sampling_rate))
        else:
            config.add_stream(lcg.streams.OutputChannel(id=ID, connections=(), device=chan['device'],
                                                        subdevice=chan['subdevice'], channel=chan['channel'],
                                                        conversionFactor=chan['factor'],
                                                        reference=chan['reference'], units=chan['units'],
                                                        stimulusFile=chan['stimfile'],
                                                        samplingRate=sampling_rate))
        ID += 1
        
    config.write(config_file)
    return True

