#!/usr/bin/env python

import os
import sys
import numpy as np
import getopt
import subprocess as sub
import lcg

stimuli_directory = 'stimuli'

def usage():
    print('\nUsage: %s [option <value>]' % os.path.basename(sys.argv[0]))
    print('\nwhere options are:\n')
    print('     -a    stimulation amplitudes (start,[stop,step])')
    print('     -d    stimulation duration (default 1 sec)')
    print('     -t    tail duration (0 pA of output after the stimulation, default 1 sec)')
    print('     -n    number of repetitions of each amplitude (default 1)')
    print('     -i    interval between repetitions (default 1 s)')
    print('\nAcquisition options:')
    print('     -I    input channel (default 0)')
    print('     -O    output channel (default 0)')
    print('     -F    sampling frequency (default 20000)')
    print('\nAdditional options:')
    print(' --with-preamble   include stability preamble.')
    print(' --no-shuffle      do not shuffle trials.')
    print(' --no-kernel       do not compute the electrode kernel.')

def main():
    try:
        opts,args = getopt.getopt(sys.argv[1:], 'hd:a:t:n:i:I:O:F:', ['help','with-preamble','no-shuffle','no-kernel'])
    except getopt.GetoptError, err:
        print(str(err))
        usage()
        sys.exit(1)

    ao = 0
    ai = 0
    samplf = 20000    # [Hz]
    with_preamble = False
    shuffle = True
    kernel = True
    nreps = 1
    duration = 1       # [s]
    interval = 1       # [s]
    tail = 1           # [s]
    stim_ampl = []     # [pA]

    for o,a in opts:
        if o in ['-h','--help']:
            usage()
            sys.exit(0)
        elif o == '-d':
            duration = float(a)
        elif o == '-a':
            for amp in a.split(','):
                stim_ampl.append(float(amp))
        elif o == '-t':
            tail = float(a)
        elif o == '-n':
            nreps = int(a)
        elif o == '-i':
            interval = float(a)
        elif o == '-I':
            ai = int(a)
        elif o == '-O':
            ao = int(a)
        elif o == '-F':
            samplf = float(a)
        elif o == '--with-preamble':
            with_preamble = True
        elif o == '--no-shuffle':
            shuffle = False
        elif o == '--no-kernel':
            kernel = False

    if len(stim_ampl) == 1:
        stim_ampl.append(stim_ampl[0])
        stim_ampl.append(1)
    elif len(stim_ampl) != 3:
        print('The amplitudes must be in the form start[,stop,step].')
        sys.exit(1)

    amplitudes = np.arange(stim_ampl[0],stim_ampl[1]+1,stim_ampl[2])
    if shuffle:
        np.random.shuffle(amplitudes)

    try:
        os.mkdir(stimuli_directory,0755)
    except:
        ok = raw_input('The directory ['+stimuli_directory+'] already exists: shall I continue [y/N] ')
        if ok != 'y':
            sys.exit(0)

    if with_preamble:
        stimulus = [[duration,1,0,0,0,0,0,0,0,0,0,1],
                    [tail,1,0,0,0,0,0,0,0,0,0,1]]
        row = 0
    else:
        stimulus = [[tail,1,0,0,0,0,0,0,0,0,0,1],
                    [duration,1,0,0,0,0,0,0,0,0,0,1],
                    [tail,1,0,0,0,0,0,0,0,0,0,1]]
        row = 1

    for i,amp in enumerate(amplitudes):
        stimulus[row][2] = amp
        lcg.writeStimFile('%s/step_%02d.stim' % (stimuli_directory,i+1), stimulus, with_preamble)

    if kernel:
        sub.call('lcg kernel -I ' + str(ai) + ' -O ' + str(ao) + ' -F ' + str(samplf), shell=True)
    sub.call('lcg vcclamp -d ' + stimuli_directory + ' -i ' + str(interval) +
              ' -I ' + str(interval) + ' -N ' + str(nreps) + ' -F ' + str(samplf), shell=True)

if __name__ == '__main__':
    main()


