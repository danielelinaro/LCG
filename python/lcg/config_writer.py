
from lxml import etree
import entities

__all__ = ['XMLConfigurationFile']

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
