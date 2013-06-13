#!/usr/bin/env python

import os
import sys
import getopt
import lcg
import subprocess as sub

stim_file = 'ramp.stim'

def usage():
    print('\nThis script injects a ramp of current.')
    print('It can be used to find the current at which the neuron starts spiking.')
    print('\nUsage: %s [option <value>]' % os.path.basename(sys.argv[0]))
    print('\nwhere options are:\n')
    print('     -h    Display this help message and exit.')
    print('     -A    Final amplitude of the ramp (in pA).')
    print('     -a    Initial amplitude of the ramp (default 0 pA).')
    print('     -d    Duration of the ramp (default 10 sec).')
    print('     -n    Number of repetitions (default 1).')
    print('     -i    Interval between repetitions (in sec).')
    print('     -F    Sampling frequency (default 20000 Hz)).')
    print('     -I    Input channel (default %s)' % os.environ['AI_CHANNEL'])
    print('     -O    Output channel (default %s)' % os.environ['AO_CHANNEL'])
    print(' --without-preamble   Do not include stability preamble.')
    print('')

def main():
    try:
        opts,args = getopt.getopt(sys.argv[1:], 'hA:a:d:n:i:I:O:F:', ['help','without-preamble'])
    except getopt.GetoptError, err:
        print(str(err))
        usage()
        sys.exit(1)

    amplitude = [0,None]         # [pA]
    duration = 10                # [s]
    reps = 1
    interval = None              # [s]
    sampling_frequency = 20000   # [Hz]
    ai = int(os.environ['AI_CHANNEL'])
    ao = int(os.environ['AO_CHANNEL'])
    with_preamble = True
    before = 0.5                 # [s]
    after = 0.5                  # [s]

    for o,a in opts:
        if o in ('-h','--help'):
            usage()
            sys.exit(0)
        elif o == '-a':
            amplitude[0] = float(a)
        elif o == '-A':
            amplitude[1] = float(a)
        elif o == '-d':
            duration = float(a)
        elif o == '-n':
            reps = int(a)
        elif o == '-i':
            interval = float(a)
        elif o == '-I':
            ai = int(a)
        elif o == '-O':
            ao = int(a)
        elif o == '-F':
            sampling_frequency = float(a)
        elif o == '--without-preamble':
            with_preamble = False

    if not amplitude[1]:
        print('You must specify the final amplitude of the ramp (-A switch).')
        sys.exit(1)

    if duration <= 0:
        print('The duration of the ramp must be positive.')
        sys.exit(1)

    if reps <= 0:
        print('The number of repetitions must be positive.')
        sys.exit(1)

    if reps > 1 and not interval:
        print('You must specify the interval between trials (-i switch).')
        sys.exit(1)

    if not with_preamble:
        stim = [[before,1,0,0,0,0,0,0,0,0,0,1]]
    else:
        stim = []
    if amplitude[0] != 0:
        stim.append([duration,-2,amplitude[0],0,0,0,0,0,0,1,0,1])
        stim.append([0,-2,amplitude[1],0,0,0,0,0,0,7,1,1])
    else:
        stim.append([duration,7,amplitude[1],0,0,0,0,0,0,0,0,1])
    stim.append([after,1,0,0,0,0,0,0,0,0,0,1])

    lcg.writeStimFile(stim_file,stim,with_preamble)
    
    sub.call('lcg kernel -I ' + str(ai) + ' -O ' + str(ao) + ' -F ' + str(sampling_frequency), shell=True)
    sub.call('lcg vcclamp -f ' + stim_file + ' -n ' + str(reps) + ' -i ' + str(interval) + ' -F ' + str(sampling_frequency), shell=True)

if __name__ == '__main__':
    main()

