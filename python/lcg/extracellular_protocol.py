#!/usr/bin/env python

import os
import sys
import getopt
import subprocess as sub
import numpy as np
import lcg

stim_file = 'pulses.stim'

def usage():
    print('\nUsage: %s [option <value>]' % os.path.basename(sys.argv[0]))
    print('\nwhere options are:\n')
    print('     -h    display this help message and exit.')
    print('     -f    frequency of pulses.')
    print('     -n    number of pulses (default 10).')
    print('     -d    duration of each pulse (default 0.1 msec).')
    print('     -a    amplitude of each pulse (default 5 V).')
    print('     -i    interval between trials (default 5 sec).')
    print('     -N    number of repetitions (default 20).')
    print('     -I    input channel (default 0).')
    print('     -O    output channel for extracellular stimulation (default 1).')
    print('')
    print(' --without-recovery-pulse    do not include a recovery pulse in the stimulation.')
    print(' --compute-kernel            run a kernel protocol for the input.')
    print('\nIn case the --compute-kernel option is specified, the following option is accepted:\n')
    print('     -o    output channel for kernel computation (default 0).')
    print('')

def main():
    try:
        opts,args = getopt.getopt(sys.argv[1:], 'hf:n:d:a:i:N:I:O:o:',
                                  ['help','compute-kernel','without-recovery-pulse'])
    except getopt.GetoptError, err:
        print str(err)
        usage()
        sys.exit(1)

    repetitions = 20      # [1]
    interval = 5          # [s]
    ai = 0
    ao = {'intra': 0, 'extra': 1}
    stim_freq = None      # [Hz]
    stim_dur = 0.1        # [ms]
    stim_amp = 5          # [V]
    npulses = 10
    with_recovery = True
    compute_kernel = False

    for o,a in opts:
        if o in ('-h','--help'):
            usage()
            sys.exit(0)
        elif o == '-f':
            stim_freq = float(a)
        elif o == '-n':
            npulses = int(a)
        elif o == '-d':
            stim_dur = float(a)
        elif o == '-i':
            interval = float(a)
        elif o == '-a':
            stim_amp = float(a)
        elif o == '-N':
            repetitions = int(a)
        elif o == '-I':
            ai = int(a)
        elif o == '-O':
            ao['extra'] = int(a)
        elif o == '-o':
            ao['intra'] = int(a)
        elif o == '--compute-kernel':
            compute_kernel = True
        elif o == '--without-recovery-pulse':
            with_recovery = False

    if not stim_freq:
        print('You must specify the stimulation frequency (-f switch).')
        sys.exit(1)

    if compute_kernel:
        if ao['intra'] == ao['extra']:
            print('The stimulation output channel [%d] should be different from the kernel output channel [%d].'
                  % (ao['intra'], ao['extra']))
            sys.exit(1)
        sub.call('lcg kernel -I ' + str(ai) + ' -O ' + str(ao['intra']), shell=True)

    sub.call('cclamprc_write -e -i -c ' + str(ai), shell=True)
    sub.call('cclamprc_write -o -f 1 -u V -c ' + str(ao['extra']), shell=True)

    lcg.writePulsesStimFile(stim_freq, stim_dur, stim_amp, npulses, withRecovery=with_recovery, filename=stim_file)
    
    sub.call('lcg vcclamp -f ' + stim_file + ' -n ' + str(repetitions) + ' -i ' + str(interval))

if __name__ == '__main__':
    main()
