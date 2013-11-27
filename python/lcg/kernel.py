#!/usr/bin/env python

import os
import sys
import getopt
import subprocess as sub
import glob
from numpy import random as rnd
import lcg

stim_file = 'kernel.stim'

def usage():
    print('')
    print('   lcg-kernel - Inject white noise into a cell to compute the kernel used')
    print('                for the Active Electrode Compensation.')
    print('')
    print('Usage: %s [option <value>]' % os.path.basename(sys.argv[0]))
    print('\nwhere options are:\n')
    print('              -h    display this help message and exit.')
    print('              -d    duration of the stimulation (default 10 sec).')
    print('              -s    standard deviation of the white noise (default 250 pA).')
    print('              -I    input channel (default %s).' % os.environ['AI_CHANNEL'])
    print('              -O    output channel (default %s).' % os.environ['AO_CHANNEL'])
    print('              -F    sampling frequency (default %s Hz).' % os.environ['SAMPLING_RATE'])
    print('              -H    holding current (default 0 pA).')
    print('  --input-factor    input conversion factor (default %s).' % os.environ['AI_CONVERSION_FACTOR_CC'])
    print(' --output-factor    output factor (default %s).' % os.environ['AO_CONVERSION_FACTOR_CC'])
    print('   --input-units    input units (default %s).' % os.environ['AI_UNITS_CC'])
    print('  --output-units    output units (default %s).' % os.environ['AO_UNITS_CC'])
    print('    -a, --append    append channel numbers to kernel.dat file (default no).')
    print('        --non-rt    do not use real-time capabilities (default no).')
    print('')

def main():
    try:
        opts,args = getopt.getopt(sys.argv[1:], 'hd:s:I:O:F:H:a',
                                  ['input-units=','output-units=','input-factor=','output-factor=',
                                   'help','append','non-rt'])
    except getopt.GetoptError, err:
        print str(err)
        usage()
        sys.exit(1)

    ai = int(os.environ['AI_CHANNEL'])
    ao = int(os.environ['AO_CHANNEL'])
    input_factor = float(os.environ['AI_CONVERSION_FACTOR_CC'])
    output_factor = float(os.environ['AO_CONVERSION_FACTOR_CC'])
    input_units = os.environ['AI_UNITS_CC']
    output_units = os.environ['AO_UNITS_CC']
    duration = 10          # [s]
    amplitude = 250        # [pA]
    sampling_rate = float(os.environ['SAMPLING_RATE'])  # [Hz]
    holding_current = 0    # [pA]
    append = False
    nonrt = False

    for o,a in opts:
        if o in ('-h','--help'):
            usage()
            sys.exit(0)
        elif o in ('-a','--append'):
            append = True
        elif o == '-d':
            duration = float(a)
        elif o == '-s':
            amplitude = float(a)
        elif o == '--output-factor':
            output_factor = double(a)
        elif o == '--input-factor':
            input_factor = double(a)
        elif o == '--output-units':
            output_units = a
        elif o == '--input-units':
            input_units = a
        elif o == '-I':
            ai = int(a)
        elif o == '-O':
            ao = int(a)
        elif o == '-F':
            sampling_rate = float(a)
        elif o == '-H':
            holding_current = float(a)
        elif o == '--non-rt':
            nonrt = True

    if input_factor <= 0. or output_factor <= 0. or sampling_rate <= 0.:
        print('Conversion factors and sampling rate must be positive.')
        sys.exit(1)

    if duration <= 0. or amplitude <= 0.:
        print('Duration and amplitude of the stimulation must be positive.')
        sys.exit(1)

    if ai < 0 or ao < 0:
        print('I/O channels must be non-negative.')
        sys.exit(1)

    if append:
        suffix = '-' + str(ai) + '-' + str(ao)
    else:
        suffix = ''

    stim = [[duration,11,0,amplitude,0,0,0,1,int(rnd.uniform(high=10000)),0,0,1],
            [1,1,0,0,0,0,0,0,0,0,0,1]]
    lcg.writeStimFile(stim_file,stim,True)
    if nonrt:
        print('Using non-realtime kernel!')
        fname = 'kernel.cfg'
        sub.call('lcg-rcwrite -e -i -c ' + str(ai) + ' --non-rt --file ' + fname + ' -f ' + str(input_factor) + ' -u ' + input_units, shell=True)
        sub.call('lcg-rcwrite -o -c ' + str(ao) + ' --non-rt --file ' + fname + ' -p ' + stim_file + ' -f ' + str(output_factor) + ' -u ' + output_units, shell=True)
        sub.call('lcg-non-rt -c ' + fname + ' -F ' + str(sampling_rate) + ' -H ' + str(holding_current), shell=True)
    else:
        sub.call('lcg-rcwrite -e -i -c ' + str(ai) + ' -f '+ str(input_factor) + ' -u ' + input_units, shell=True)
        sub.call('lcg-rcwrite -o -c ' + str(ao) + ' -f ' + str(output_factor) + ' -u ' + output_units, shell=True)
        sub.call('lcg vcclamp -f ' + stim_file + ' -F ' + str(sampling_rate) + ' -H ' + str(holding_current), shell=True)

    files = glob.glob('*.h5')
    files.sort()
    data_file = files[-1]
    lcg.computeElectrodeKernel(data_file)
    try:
        os.remove('kernel' + suffix + '.dat')
    except:
        pass
    os.symlink(data_file[:-3] + '_kernel.dat', 'kernel' + suffix + '.dat')

if __name__ == '__main__':
    main()
