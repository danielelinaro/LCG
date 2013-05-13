from lxml import etree

__all__ = ['lcg_xml']

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
