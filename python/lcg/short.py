#!/usr/bin/env python

import os
import sys
import subprocess as sub
import lcg

default_xml_file = 'short.xml'
progname = os.path.basename(sys.argv[0])

def write_xml_file(tend=60,srate=os.environ['SAMPLING_RATE'],xml_file=default_xml_file):
    rec = lcg.entities.H5Recorder(id=0, connections=(), compress=True)
    short = lcg.entities.Short(id=1, connections=(0), deviceFile=os.environ['COMEDI_DEVICE'],
                               inputSubdevice=os.environ['AI_SUBDEVICE'],
                               outputSubdevice=os.environ['AO_SUBDEVICE'],
                               readChannel=os.environ['AI_CHANNEL_CC'],
                               writeChannel=os.environ['AO_CHANNEL_CC'],
                               inputConversionFactor=1, outputConversionFactor=1,
                               inputUnits='V', outputUnits='V',
                               inputRange=os.environ['RANGE'],
                               reference=os.environ['GROUND_REFERENCE'],
                               resetOutput=os.environ['LCG_RESET_OUTPUT'])
    config = lcg.XMLConfigurationFile(srate,tend)
    config.add_entity(rec)
    config.add_entity(short)
    config.write(xml_file)

def main():
    if len(sys.argv) == 1:
        tend = 60
    else:
        if sys.argv[1] in ('-h','--help','help'):
            print('This script reads from input channel %s and outputs, step by step,' % os.environ['AI_CHANNEL_CC'])
            print('the same value to output channel %s.' % os.environ['AO_CHANNEL_CC'])
            print('')
            print('Usage: %s [duration]' % progname)
            print('')
            print('If not specified, the duration is 60 seconds.')
            sys.exit(0)
        else:
            try:
                tend = float(sys.argv[1]) 
            except:
                print('%s: unknown option/argument \'%s\'.' % (progname,sys.argv[1]))
                print('Type \'%s -h\' for help.' % progname)
                sys.exit(2)
            if tend <= 0:
                print('The duration of the recording must be positive.')
                sys.exit(1)
    
    write_xml_file(tend,xml_file=default_xml_file)
    sub.call('lcg-experiment -c ' + default_xml_file, shell=True)

if __name__ == '__main__':
    main()
