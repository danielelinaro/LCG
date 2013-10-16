#!/usr/bin/env python
'''
 Writes configuration files for lcg-non-rt and lcg-vcclamp. 
 If no file is specified, it writes to the .cclamprc or .non-rt
'''

import os
import sys
import getopt
import subprocess as sub
import glob
import lcg
import re

def usage():
    print('\nUsage: %s [option <value>]' % os.path.basename(sys.argv[0]))
    print('\nwhere options are:\n')
    print('     -h    display this help message and exit.')
    print('     -i    (--input) write input entry.')
    print('     -o    (--output) write output entry.')
    print('     -c    channel number.')
    print('     -s    subdevice number (default %s for AI, %s for AO).'%(os.environ['AI_SUBDEVICE'],os.environ['AO_SUBDEVICE']))
    print('     -u    units (default %s for AI, %s for AO).'%(os.environ['INPUT_UNITS'],os.environ['OUTPUT_UNITS']))
    print('     -e    (--erase) erase existing file (default no).')
    print('     -r    ground reference (default %s).'%(os.environ['GROUND_REFERENCE']))
    print('     -f    filename instead of the default (.cclamprc and .lcg-non-rt).')
    print('     --non-rt Write non-rt configuration file.')
    print('     -p    path of the stimulus files to use in non-rt configuration (optional)')
    print('WARNING: This script does not check that there are no repeated channels.')
    print('         In non-lcg files the -append option does not append the channels, ')
    print('these must be re-written every time.\n')

def find_entity_number(filename,input_chan):
    string = 'Output'
    n = -1
    if input_chan:
        string = 'Input'
    try:
        for line in open(filename):
            if string in line:
                n = int(re.findall(r'\d+',line)[0])
    except IOError:
        print('File {0} does not exist.\n'.format(filename))
    return n

def main():
    try:
        opts,args = getopt.getopt(sys.argv[1:], 'heioc:p:f:s:u:e:r', ['help','input','output','erase','non-rt'])
    except getopt.GetoptError, err:
        print str(err)
        usage()
        sys.exit(1)

    nonrt = False 
    filename = None
    input_chan = None
    append = 'a'
    subdevice = None
    channel = None
    device = os.environ['COMEDI_DEVICE']
    reference = os.environ['GROUND_REFERENCE']
    units = None
    conversion_factor = None
    append = 'a'
    stimfiles = None

    for o,a in opts:
        if o in ('-h','--help'):
            usage()
            sys.exit(0)
        elif o in ('-i','--input'):
            input_chan=True 
        elif o in ('-o','--output'):
            input_chan=False 
        elif o in ('-e','--erase'):
            append='w'
        elif o in ('-c'):
            channel = a
        elif o in ('-c'):
            subdevice = a
        elif o in ('-c'):
             conversion_factor= a
        elif o in ('-u'):
            u = a
        elif o in ('-r'):
            reference = a
        elif o in ('-f'):
            filename = a
        elif o in ('-p'):
            stimfiles = a
        elif o in ('--non-rt'):
            nonrt = True
    
    if input_chan is None:
        print("You must specify whether you are writing an input or output channel.")
        usage()
        sys.exit(1)
    if channel is None:
        print("You must specify whether you are writing an input or output channel.")
        usage()
        sys.exit(1)

    if input_chan:
        header='[AnalogInput{0:d}]\n'
        if subdevice is None:
            subdevice = os.environ['AI_SUBDEVICE']
        if conversion_factor is None:
            conversion_factor = os.environ['AI_CONVERSION_FACTOR']
        if units is None:
            units = os.environ['INPUT_UNITS']
    else:
        header='[AnalogOutput{0:d}]\n'
        if subdevice is None:
            subdevice = os.environ['AO_SUBDEVICE']
        if conversion_factor is None:
            conversion_factor = os.environ['AO_CONVERSION_FACTOR']
        if units is None:
            units = os.environ['OUTPUT_UNITS']

    if not nonrt:
        if filename is None:
            filename = os.environ["HOME"]+'/.cclamprc' 
        n = find_entity_number(filename,input_chan)
        if append in ['a']:
            n += 1
        else:
            n = 0
        with file(filename,append) as fd:
            fd.write(header.format(n))
            fd.write('device = {0}\n'.format(device))
            fd.write('range = [-10,+10]\n')
            fd.write('subdevice = {0}\n'.format(subdevice))
            fd.write('channel = {0}\n'.format(channel))
            fd.write('conversionFactor = {0}\n'.format(conversion_factor))
            fd.write('reference = {0}\n'.format(reference))
            fd.write('units = {0}\n'.format(units))
            fd.write('\n')
    else:
        if filename is None:
            filename = os.environ["HOME"]+'/.lcg-non-rt'
        header = '[AnalogOutput]\n'
        if input_chan:
            header = '[AnalogInput]\n'
        with file(filename,append) as fd:
            fd.write(header)
            fd.write('device = {0}\n'.format(device))
            fd.write('range = [-10,+10]\n')
            fd.write('subdevice = {0}\n'.format(subdevice))
            fd.write('channels = {0}\n'.format(channel))
            fd.write('conversionFactors = {0}\n'.format(conversion_factor))
            fd.write('reference = {0}\n'.format(reference))
            fd.write('units = {0}\n'.format(units))
            if (not stimfiles is None) and (not input_chan):
                fd.write('stimfiles = {0}\n'.format(stimfiles))
            fd.write('\n')
    return

        
