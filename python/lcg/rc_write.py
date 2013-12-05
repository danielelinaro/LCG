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
    print('           -h   display this help message and exit.')
    print(' -i,  --input   write input entry.')
    print(' -o, --output   write output entry.')
    print('           -c   channel number.')
    print('           -f   conversion factor.')
    print('           -s   subdevice number (default %s for AI, %s for AO).' % (os.environ['AI_SUBDEVICE'],os.environ['AO_SUBDEVICE']))
    print('           -u   units (default %s for AI, %s for AO).' % (os.environ['AI_UNITS_CC'],os.environ['AO_UNITS_CC']))
    print(' -e,  --erase   erase existing file (default no).')
    print('           -r   ground reference (default %s).' % (os.environ['GROUND_REFERENCE']))
    print('       --file   filename instead of the default (.cclamprc or .lcg-non-rt).')
    print('     --non-rt   write non-rt configuration file.')
    print('           -p   path of the stimulus files to use in non-rt configuration (optional).')
    print('')
    print('WARNING: This script does not check that there are no repeated channels.')
    print('         In configuration files to be used by lcg-non-rt, the --append option')
    print('         does not append the channels, they must be re-written every time.')
    print('')

def get_next_id(filename, io_type):
    n = 0
    try:
        with open(filename,'r') as fid:
            for line in fid:
                if io_type in line:
                    n = int(re.findall(r'\d+',line)[0]) + 1
    except IOError:
        print('File %s does not exist.' % filename)
    return n

def main():
    try:
        opts,args = getopt.getopt(sys.argv[1:], 'heioc:p:f:s:u:e:r', ['help','input','output','erase','non-rt','file='])
    except getopt.GetoptError, err:
        print str(err)
        usage()
        sys.exit(1)

    nonrt = False 
    filename = None
    input_chan = False
    output_chan = False
    subdevice = None
    channel = None
    device = os.environ['COMEDI_DEVICE']
    reference = os.environ['GROUND_REFERENCE']
    units = None
    conversion_factor = None
    io_mode = 'a'
    stimfiles = None

    for o,a in opts:
        if o in ('-h','--help'):
            usage()
            sys.exit(0)
        elif o in ('-i','--input'):
            input_chan = True
            io_type = 'Input'
        elif o in ('-o','--output'):
            output_chan = True 
            io_type = 'Output'
        elif o in ('-e','--erase'):
            io_mode = 'w'
        elif o == '-c':
            channel = a
        elif o == '-s':
            subdevice = a
        elif o == '-f':
            conversion_factor= a
        elif o == '-u':
            units = a
        elif o == '-r':
            reference = a
        elif o == '--file':
            filename = a
        elif o == '-p':
            stimfiles = a
        elif o == '--non-rt':
            nonrt = True
    
    if not input_chan and not output_chan:
        print('You must specify whether you are writing an input (--input switch) or an output channel (--output switch).')
        sys.exit(1)

    if input_chan and output_chan:
        print('You must specify only one of --input or --output.')
        sys.exit(1)

    if channel is None:
        print('You must specify the channel number (-c switch).')
        sys.exit(1)

    if input_chan:
        header='[AnalogInput{0:d}]\n'
        if subdevice is None:
            subdevice = os.environ['AI_SUBDEVICE']
        if conversion_factor is None:
            conversion_factor = os.environ['AI_CONVERSION_FACTOR_CC']
        if units is None:
            units = os.environ['AI_UNITS_CC']
    else:
        header='[AnalogOutput{0:d}]\n'
        if subdevice is None:
            subdevice = os.environ['AO_SUBDEVICE']
        if conversion_factor is None:
            conversion_factor = os.environ['AO_CONVERSION_FACTOR_CC']
        if units is None:
            units = os.environ['AO_UNITS_CC']

    if not nonrt:
        if filename is None:
            filename = os.environ['HOME']+'/.cclamprc' 
        if io_mode == 'a':
            n = get_next_id(filename,io_type)
        else:
            n = 0
        with file(filename,io_mode) as fd:
            fd.write(header.format(n))
            fd.write('device = {0}\n'.format(device))
            fd.write('range = {0}\n'.format(os.environ['RANGE']))
            fd.write('subdevice = {0}\n'.format(subdevice))
            fd.write('channel = {0}\n'.format(channel))
            fd.write('conversionFactor = {0}\n'.format(conversion_factor))
            fd.write('reference = {0}\n'.format(reference))
            fd.write('units = {0}\n'.format(units))
            fd.write('\n')
    else:
        if filename is None:
            filename = os.environ['HOME']+'/.lcg-non-rt'
        if input_chan:
            header = '[AnalogInput]\n'
        else:
            header = '[AnalogOutput]\n'
        with file(filename,io_mode) as fd:
            fd.write(header)
            fd.write('device = {0}\n'.format(device))
            fd.write('range = {0}\n'.format(os.environ['RANGE']))
            fd.write('subdevice = {0}\n'.format(subdevice))
            fd.write('channels = {0}\n'.format(channel))
            fd.write('conversionFactors = {0}\n'.format(conversion_factor))
            fd.write('reference = {0}\n'.format(reference))
            fd.write('units = {0}\n'.format(units))
            if output_chan and stimfiles is not None:
                fd.write('stimfiles = {0}\n'.format(stimfiles))
            fd.write('\n')

if __name__ == '__main__':
    main()

