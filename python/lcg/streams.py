#
# This file contains definitions of all the streams, so that they can be used
# with the class that writes configuration files.
#
# Author: Daniele Linaro - December 2013
#

from config_writer import XMLEntry

class Stream (XMLEntry):
    def __init__(self, name, id, connections):
        super(Stream,self).__init__('Stream',name,id,connections)

class InputChannel (Stream):
    def __init__(self, id, connections, device, subdevice, channel,
                 conversionFactor, range, reference, units, samplingRate):
        super(InputChannel,self).__init__('InputChannel', id, connections)
        self.add_parameter('device', device)
        self.add_parameter('subdevice', subdevice)
        self.add_parameter('channel', channel)
        self.add_parameter('conversionFactor', conversionFactor)
        self.add_parameter('range', range)
        self.add_parameter('reference', reference)
        self.add_parameter('units', units)
        self.add_parameter('samplingRate', samplingRate)
        
class OutputChannel (Stream):
    def __init__(self, id, connections, device, subdevice, channel,
                 conversionFactor, reference, units, stimulusFile, samplingRate):
        super(OutputChannel,self).__init__('OutputChannel', id, connections)
        self.add_parameter('device', device)
        self.add_parameter('subdevice', subdevice)
        self.add_parameter('channel', channel)
        self.add_parameter('conversionFactor', conversionFactor)
        self.add_parameter('reference', reference)
        self.add_parameter('units', units)
        self.add_parameter('stimfile', stimulusFile)
        self.add_parameter('samplingRate', samplingRate)
