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
    print('            -h    Display this help message and exit.')
    print('            -A    Final amplitude of the ramp (in pA).')
    print('            -a    Initial amplitude of the ramp (default 0 pA).')
    print('            -d    Duration of the ramp (default 10 sec).')
    print('            -n    Number of repetitions (default 2).')
    print('            -i    Interval between repetitions (default 20 sec).')
    print('            -F    Sampling frequency (default %s Hz).' % os.environ['SAMPLING_RATE'])
    print('            -I    Input channel (default %s).' % os.environ['AI_CHANNEL'])
    print('            -O    Output channel (default %s).' % os.environ['AO_CHANNEL'])
    print('          --rt    Use real-time system (yes or no, default %s).' % os.environ['LCG_REALTIME'])
    print('   --no-kernel    Do not compute the electrode kernel.')
    print(' --no-preamble    Do not include stability preamble.')
    print('')

def main():
    try:
        opts,args = getopt.getopt(sys.argv[1:], 'hA:a:d:n:i:I:O:F:', ['help','no-kernel','no-preamble','rt='])
    except getopt.GetoptError, err:
        print(str(err))
        usage()
        sys.exit(1)

    amplitude = [0,None]         # [pA]
    duration = 10                # [s]
    reps = 2
    interval = 20                # [s]
    sampling_frequency = float(os.environ['SAMPLING_RATE'])   # [Hz]
    ai = int(os.environ['AI_CHANNEL'])
    ao = int(os.environ['AO_CHANNEL'])
    with_preamble = True
    with_kernel = True
    before = 0.5                 # [s]
    after = 0.5                  # [s]
    realtime = os.environ['LCG_REALTIME']

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
        elif o == '--no-kernel':
            with_kernel = False
        elif o == '--no-preamble':
            with_preamble = False
        elif o == '--rt':
            realtime = a

    if not amplitude[1]:
        print('You must specify the final amplitude of the ramp (-A switch).')
        sys.exit(1)

    if duration <= 0:
        print('The duration of the ramp must be positive.')
        sys.exit(1)

    if reps <= 0:
        print('The number of repetitions must be positive.')
        sys.exit(1)

    if reps > 1 and interval < 0:
        print('The interval between repetitions must be non-negative.')
        sys.exit(1)

    stimulus = 'lcg stimgen -o %s ' % stim_file
    if with_preamble:
        stimulus += 'dc -d 0.5 0 dc -d 0.01 -- -300 dc -d 0.5 0 dc -d 0.6 -- -100 dc -d 1 0 '
    else:
        stimulus += 'dc -d %g 0 ' % before
    if amplitude[0] != 0:
        stimulus += 'dc -p -d %g -- %g ramp -E -- %g ' % (duration,amplitude[0],amplitude[1]-amplitude[0])
    else:
        stimulus += 'ramp -d %g -- %g ' % (duration,amplitude[1])
    stimulus += 'dc -d %g 0' % after

    sub.call(stimulus, shell=True)
    if with_kernel:
        sub.call('lcg kernel -I %d -O %d -F %g --rt %s' % (ai,ao,sampling_frequency,realtime), shell=True)
    sub.call('lcg stimulus -I %d -O %d -s %s -n %d -i %g -F %g --rt %s' % (ai,ao,stim_file,reps,interval,sampling_frequency,realtime), shell=True)

if __name__ == '__main__':
    main()
