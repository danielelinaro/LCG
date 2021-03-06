#!/usr/bin/env python

import os
import sys
import getopt
import lcg
import subprocess as sub

stim_file = 'pulse.stim'

def usage():
    print('\nThis script injects a single pulse of DC current.')
    print('\nUsage: %s [option <value>]' % os.path.basename(sys.argv[0]))
    print('\nwhere options are:\n')
    print('          -h    Display this help message and exit.')
    print('          -a    Amplitude of the stimulation (in pA).')
    print('          -d    Duration of the stimulation (in ms).')
    print('          -n    Number of repetitions.')
    print('          -i    Interval between repetitions (in sec).')
    print('          -B    Interval before the application of the stimulus (default 0.5 sec).')
    print('          -A    Interval after the application of the stimulus (default 0.5 sec).')
    print('          -F    Sampling frequency (default %s Hz).' % os.environ['SAMPLING_RATE'])
    print('          -I    Input channel (default %s)' % os.environ['AI_CHANNEL'])
    print('          -O    Output channel (default %s)' % os.environ['AO_CHANNEL'])
    print('        --rt    Use real-time system (yes or no, default %s)' % os.environ['LCG_REALTIME'])
    print(' --no-kernel    Do not compute the kernel.')
    print('')

def main():
    try:
        opts,args = getopt.getopt(sys.argv[1:], 'ha:d:n:i:I:O:F:B:A:', ['help','no-kernel','rt=','no-preamble'])
    except getopt.GetoptError, err:
        print(str(err))
        usage()
        sys.exit(1)

    before = 0.5                 # [s]
    after = 0.5                  # [s]
    amplitude = None             # [pA]
    duration = None              # [s]
    reps = 1
    interval = None              # [s]
    sampling_frequency = float(os.environ['SAMPLING_RATE'])   # [Hz]
    ai = int(os.environ['AI_CHANNEL'])
    ao = int(os.environ['AO_CHANNEL'])
    kernel = True
    realtime = os.environ['LCG_REALTIME'].lower()
    for o,a in opts:
        if o in ('-h','--help'):
            usage()
            sys.exit(0)
        elif o == '-a':
            amplitude = float(a)
        elif o == '-d':
            duration = float(a)*1e-3
        elif o == '-B':
            before = float(a)
        elif o == '-A':
            after = float(a)
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
            kernel = False
        elif o == '--rt':
            realtime = a.lower()

    if not amplitude:
        print('You must specify the amplitude of the stimulation (-a switch).')
        sys.exit(1)

    if not duration:
        print('You must specify the duration of the stimulation (-d switch).')
        sys.exit(1)

    if duration <= 0:
        print('The duration of the stimulation must be positive.')
        sys.exit(1)

    if reps <= 0:
        print('The number of repetitions must be positive.')
        sys.exit(1)

    if reps > 1 and not interval:
        print('You must specify the interval between trials (-i switch).')
        sys.exit(1)

    if before < 0:
        print('The time before the stimulation must be greater or equal than zero.')
        sys.exit(1)

    if after < 0:
        print('The time after the stimulation must be greater or equal than zero.')
        sys.exit(1)

    stimgen = 'lcg stimgen -o %s ' % stim_file
    if before > 0:
        stimgen += 'dc -d %f 0 ' % before
    stimgen += 'dc -d %f -- %f ' % (duration,amplitude)
    if after > 0:
        stimgen += 'dc -d %f 0' % after

    sub.call(stimgen, shell=True)
    if kernel:
        sub.call('lcg kernel -I %d -O %d -F %g --rt %s' % (ai,ao,sampling_frequency,realtime), shell=True)
    sub.call('lcg stimulus -I %d -O %d -s %s -n %d -i %g -F %g --rt %s' % (ai,ao,stim_file,reps,interval,sampling_frequency,realtime), shell=True)

if __name__ == '__main__':
    main()


