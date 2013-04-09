#!/usr/bin/env python
import os
import sys
import numpy as np
import getopt
import lcg
from lxml import etree

#import xml.etree.ElementTree as etree
#from xml.dom import minidom


def usage():
    print('\nUsage: %s mode [option <value>]' % os.path.basename(sys.argv[0]))
    print('\nwhere the modes are:\n')
    print('     steps             current steps are used to generate repetitive firing\n')
    print('     frequency-clamp   repetitive firing is stabilized using a PID controller\n')
    print('     steps-PID         a PID fixes the current before delivering the pulses')
    print('     noise             PRC is generated using noise injection\n')
    print('\n\nThe global options are:\n')
    print('     -a    stimulation amplitude (pA)')
    print('     -n    number of repetitions (default 100)')
    print('     -i    interval between repetitions (default 1 s)')
    print('\nAcquisition options:')
    print('     -I    input channel (default 0)')
    print('     -O    output channel (default 0)')
    print('     -F    sampling frequency (default 20000)')
    print('\nAdditional options:')

def add_xml_elements(group,parlist):
    '''
    Appends an xml Element to an etree.Element
    '''
    for a,o in parlist.iteritems():
        tmp = etree.SubElement(group, a)
        tmp.text = str(o)
def add_xml_parameters(group,parlist):
    '''
    Appends parameters to an etree.Element
    '''
    parameters = etree.SubElement(group,"parameters")
    add_xml_elements(parameters,parlist)

def main():
    try:
        opts,args = getopt.getopt(sys.argv[1:], 'h', ['help'])
    except getopt.GetoptError, err:
        print(str(err))
        usage()
        sys.exit(1)

    for o,a in opts:
        if o in ['-h','--help']:
            usage()
            sys.exit(0)
    
    tend = 250
    srate = 30000

    # RealNeuron parameters
    threshold = -30
    kernel_file = 'kernel.dat'
    ai = [0]
    ao = [0]
    # Generate configuration file
    root = etree.Element("dynamicclamp")
    e_group = etree.SubElement(root,"entities")
    simulation_parameters = etree.SubElement(root,"simulation")
    add_xml_elements(simulation_parameters,{'rate':srate,
                                              'tend':tend})
    # Insert entities
    id_cnt = 0
    recorder = etree.SubElement(e_group,"entity")
    add_xml_elements(recorder,{'name':'H5Recorder',
                               'id':id_cnt})
    add_xml_parameters(recorder,{'compress':True})

    ++id_cnt
    neuron = etree.SubElement(e_group,"entity")
    add_xml_elements(neuron,{'name':'RealNeuron',
                               'id':id_cnt})
    add_xml_parameters(neuron,{'spikeThreshold':threshold,
                               'V0':-65,
                               'kernelFile':kernel_file,
                               'deviceFile':'dev/comedi0',
                               'inputSubdevice':os.environ['AI_SUBDEVICE'],
                               'outputSubdevice':os.environ['AO_SUBDEVICE'],
                               'inputRange':[-10,10],
                               'readChannel':ai[0],
                               'writeChannel':ao[0],
                               'inputConversionFactor':os.environ['AI_CONVERSION_FACTOR'],
                               'outputConversionFactor':os.environ['AO_CONVERSION_FACTOR'],
                               'holdLastValue':True,
                               'reference':os.environ['GROUND_REFERENCE']})
    
    
    
    # Write configuration file
    dclamp_tree = etree.ElementTree(root) 
    dclamp_tree.write('tmp.xml',pretty_print=True)

    

if __name__ == '__main__':
    main()

