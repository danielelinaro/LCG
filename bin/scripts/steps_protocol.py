#!/usr/bin/env python

import os
import sys
import numpy as np
import getopt
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
    print('     -I    input channel (default 0)')
    print('     -O    output channel (default 0)')
    print(' --without-preamble    do not include stability preamble.')

def main():
    try:
        opts,args = getopt.getopt(sys.argv[1:], 'hd:a:t:n:i:I:O:', ['help','without-preamble'])
    except getopt.GetoptError, err:
        print(str(err))
        usage()
        sys.exit(1)

    ao = 0
    ai = 0
    with_preamble = True
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
        elif o == '--without-preamble':
            with_preamble = False
    
    if len(stim_ampl) == 1:
        stim_ampl.append(stim_ampl[0])
        stim_ampl.append(1)
    elif len(stim_ampl) != 3:
        print('The amplitudes must be in the form start[,stop,step].')
        sys.exit(1)

    os.system('kernel_protocol -I ' + str(ai) + ' -O ' + str(ao))

    amplitudes = np.arange(stim_ampl[0],stim_ampl[1]+1,stim_ampl[2])
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

    os.system('cclamp -d ' + stimuli_directory + ' -i ' + str(interval) +
              ' -I ' + str(interval) + ' -N ' + str(nreps))

if __name__ == '__main__':
    main()


