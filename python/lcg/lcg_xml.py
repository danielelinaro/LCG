
from lxml import etree
import entities

__all__ = ['lcg_xml','XMLConfigurationFile']

class lcg_xml():
    """
    Class for creating configuration files.
    This class is a part of lcg.
    """
    def __init__(self, srate=20000, tend=30):
        '''
        Initializes a configuration_file.
        '''
        self.root = etree.Element('lcg')
        self.entities = etree.SubElement(self.root,'entities')
        self.simulation = etree.SubElement(self.root,'simulation')
        self._add_elements(self.simulation,{'rate':srate,
                                            'tend':tend })
        self._entities = []
        self.ID = 0

    def _add_elements(self,group,parlist):
        '''
        Appends an xml Element to an etree.Element
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
        Base function to use 
        Appends list of numbers the SubElement "connections" of an etree.Element
        '''
        if len(connections):
            connection = entity.find('connections')
            if connection is None:
                connection = etree.SubElement(entity, 'connections')
            if not connection.text is None:
                previous_conn = [str(ii) for ii in connection.text.split(',')]
                connections = np.unique(np.array([previous_conn, connections])).flatten()
            connection.text = ",".join(map(str,connections))

    def write(self,filename):
        '''
        Saves the file.
        '''
        tree = etree.ElementTree(self.root)
        tree.write(filename, pretty_print=True)

    def add_H5Recorder(self, ID = None, compress=True):
        '''
        Adds a H5Recorder entity.
        '''
        if ID is None:
            ID = self.ID
        self._entities.append(etree.SubElement(self.entities,"entity"))
        self._add_elements(self._entities[-1],{'name':'H5Recorder',
                                              'id':ID})
        self._add_parameters(self._entities[-1],{'compress':True})
        self.ID += 1

    def add_Waveform(self, ID = None, filename = None, units = 'pA', triggered = False):
        '''
        Adds a Waveform entity.
        '''
        if ID is None:
            ID = self.ID
        self._entities.append(etree.SubElement(self.entities,"entity"))
        self._add_elements(self._entities[-1],{'name':'Waveform',
                                              'id':ID})
        self._add_parameters(self._entities[-1],{'filename':filename,
                                                'units':units,
                                                'triggered':False})
        self.ID += 1

    def add_RealNeuron(self, ID = None, threshold = -10, 
                       V0 = -65, kernelFile = None,
                       deviceFile='/dev/comedi0',
                       inputSubdevice=None,outputSubdevice=None,
                       inputRange='[-10,+10]', ai=0, ao=0,
                       inputConversionFactor=None, outputConversionFactor=None,
                       holdLastValue=True,reference=None):
        '''
        Adds a RealNeuron entity.
        '''
        if ID is None:
            ID = self.ID
        self._entities.append(etree.SubElement(self.entities,"entity"))
        self._add_elements(self._entities[-1],{'name':'RealNeuron',
                                              'id':ID})
        par = {'spikeThreshold':threshold,
               'V0':V0,
               'kernelFile':kernelFile,
               'deviceFile':'/dev/comedi0',
               'inputSubdevice':inputSubdevice,
               'outputSubdevice':outputSubdevice,
               'inputRange': [-10,10],
               'readChannel': ai,
               'writeChannel': ao,
               'inputConversionFactor':inputConversionFactor,
               'outputConversionFactor':outputConversionFactor,
               'holdLastValue':True,
               'reference':reference}
        for p,v in zip(['inputSubdevice','outputSubdevice',
                        'inputConversionFactor', 'outputConversionFactor',
                        'reference'],
                       ['AI_SUBDEVICE','AO_SUBDEVICE',
                        'AI_CONVERSION_FACTOR', 'AO_CONVERSION_FACTOR',
                        'REFERENCE']):
            if par[p] is None:
                try:
                    par[p] = os.environ[v]
                except:
                    print('Environmental variable %s not defined.'%v)

        self._add_parameters(self._entities[-1],par)
        self.ID += 1
    def add_AnalogInput(self, ID = None,  
                        deviceFile='/dev/comedi0',
                        inputSubdevice=None,
                        inputRange='[-10,+10]', 
                        ai=0,
                        inputConversionFactor=None,
                        reference=None):
        '''
        Adds a AnalogInput entity.
        '''
        if ID is None:
            ID = self.ID
        self._entities.append(etree.SubElement(self.entities,"entity"))
        self._add_elements(self._entities[-1],{'name':'AnalogInput',
                                              'id':ID})
                
        par = {'deviceFile':'/dev/comedi0',
               'inputSubdevice':inputSubdevice,
               'inputRange': '[-10,+10]',
               'readChannel': ai,
               'inputConversionFactor':inputConversionFactor,
               'reference':reference}
        for p,v in zip(['inputSubdevice',
                        'inputConversionFactor',
                        'reference'],
                       ['AI_SUBDEVICE',
                        'AI_CONVERSION_FACTOR',
                        'REFERENCE']):
            if par[p] is None:
                try:
                    par[p] = os.environ[v]
                except:
                    print('Environmental variable %s not defined.'%v)

        self._add_parameters(self._entities[-1],par)
        self.ID += 1
    def add_AnalogOutput(self, ID = None,        
                       deviceFile='/dev/comedi0',
                       outputSubdevice=None,
                       ao=0, outputConversionFactor=None,
                       reference=None):
        '''
        Adds a AnalogOutput entity.
        '''
        if ID is None:
            ID = self.ID
        self._entities.append(etree.SubElement(self.entities,"entity"))
        self._add_elements(self._entities[-1],{'name':'AnalogOutput',
                                              'id':ID})
        par = {'deviceFile':'/dev/comedi0',
               'outputSubdevice':outputSubdevice,
               'writeChannel': ao,
               'outputConversionFactor':outputConversionFactor,
               'reference':reference}
        for p,v in zip(['outputSubdevice',
                        'outputConversionFactor',
                        'reference'],
                       ['AO_SUBDEVICE',
                        'AO_CONVERSION_FACTOR',
                        'REFERENCE']):
            if par[p] is None:
                try:
                    par[p] = os.environ[v]
                except:
                    print('Environmental variable %s not defined.'%v)

        self._add_parameters(self._entities[-1],par)
        self.ID += 1

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
        self._xml_entities = etree.SubElement(self._xml_root,'entities')
        self._xml_simulation = etree.SubElement(self._xml_root,'simulation')
        self._add_elements(self._xml_simulation,{'rate':sampling_rate,'tend':trial_duration})
        self._entities = []

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
        self._entities.append(etree.SubElement(self._xml_entities,"entity"))
        self._add_elements(self._entities[-1], {'name': entity.name, 'id': entity.id})
        self._add_parameters(self._entities[-1], entity.parameters)
        self._add_connections(self._entities[-1], entity.connections)

    def write(self,filename):
        '''
        Saves the file.
        '''
        tree = etree.ElementTree(self._xml_root)
        tree.write(filename, pretty_print=True)

if __name__ == '__main__':
    f = XMLConfigurationFile(20000, 10)
    f.add_entity(entities.H5Recorder(1, (), True, 'spam.h5'))
    f.add_entity(entities.AnalogInput(2, 0, '/dev/comedi', 0, 0, 100, '[-10,+10]', 'GRSE', 'mV'))
    f.add_entity(entities.AnalogOutput(3, (), '/dev/comedi', 1, 0, 0.001, 'GRSE', 'pA'))
    f.add_entity(entities.Waveform(4, (0,3), 'spam.stim', 'pA', False))
    f.write('spam.xml')
