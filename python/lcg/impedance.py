#!/usr/bin/env python

import os
import sys
import getopt
import lcg
import subprocess as sub
import numpy as np

def usage():
    print('This script injects subthreshold currents into a neuron to measure its impedance.')
    print('\nUsage: %s [option <value>]' % os.path.basename(sys.argv[0]))
    print('\nwhere options are:\n')
    print('     -h    Display this help message and exit.')
    print('     -N    Number of cycles (default 50).')
    print('     -f    Tested frequencies (comma separated values, default 0.1,0.2,0.5,1,2,5,10,20,30,50,80,100,200 Hz).')
    print('     -a    Amplitude of the stimulation (default 50 pA).')
    print('     -d    (Minimal) duration of the stimulation (default 2 s).')
    print('     -D    (Maximal) duration of the stimulation (default 100 s).')
    print('     -n    Number of repetitions (default 1).')
    print('     -i    Interval between repetitions (default 1 sec).')
    print('     -F    Sampling frequency (default %s Hz).' % os.environ['SAMPLING_RATE'])
    print('     -I    Input channel (default %s).' % os.environ['AI_CHANNEL'])
    print('     -O    Output channel (default %s).' % os.environ['AO_CHANNEL'])
    print('   --rt    Use real-time system (yes or no, default %s).' % os.environ['LCG_REALTIME'])
    print('')

def main():
    try:
        opts,args = getopt.getopt(sys.argv[1:], 'hN:f:a:d:D:n:i:I:O:F:', ['help','rt='])
    except getopt.GetoptError, err:
        print(str(err))
        sys.exit(1)

    ncycles = 50
    frequencies = [0.1,0.2,0.5,1.,2.,5.,10.,20.,30.,50.,80.,100.,200.] # [Hz]
    amplitude = 50. # [pA]
    min_duration = 2. # [s]
    max_duration = 100. # [s]
    nreps = 1
    interval = 1 # [s]
    sampling_rate = float(os.environ['SAMPLING_RATE'])
    ai = os.environ['AI_CHANNEL']
    ao = os.environ['AO_CHANNEL']
    realtime = os.environ['LCG_REALTIME']

    for o,a in opts:
        if o in ('-h','--help'):
            usage()
            sys.exit(0)
        elif o == '-N':
            ncycles = int(a)
            if ncycles <= 0:
                print('Number of cycles (-N option) must be positive.')
                sys.exit(2)
        elif o == '-f':
            frequencies = [float(s) for s in a.split(',')]
            if any(map(lambda x: x<0, frequencies)):
                print('All frequencies must be positive.')
                sys.exit(3)
        elif o == '-a':
            amplitude = float(a)
            if amplitude <= 0:
                print('Amplitude (-a option) must be positive.')
                sys.exit(4)
        elif o == '-d':
            min_duration = float(a)
            if min_duration <= 0:
                print('Minimal duration (-d option) must be positive.')
                sys.exit(5)
        elif o == '-D':
            min_duration = float(a)
            if min_duration <= 0:
                print('Maximal duration (-D option) must be positive.')
                sys.exit(6)
        elif o == '-n':
            nreps = int(a)
            if nreps <= 0:
                print('Number of repetitions (-n option) must be positive.')
                sys.exit(7)
        elif o == '-i':
            interval = float(a)
            if interval <= 0:
                print('Interval between repetitions (-i option) must be positive.')
                sys.exit(8)
        elif o == '-F':
            sampling_rate = float(a)
        elif o == '-I':
            ai = a
        elif o == '-O':
            ao = a
        elif o == '--rt':
            realtime = a

    sub.call('lcg kernel', shell=True)

    for i in range(nreps):
        for f in frequencies:
            f = sampling_rate / round(sampling_rate / f)
            duration = ncycles / f
            if duration > max_duration:
                duration = round(max_duration*f)/f
            elif duration < min_duration:
                duration = round(min_duration*f)/f
            cmd = 'lcg stimgen dc -d 0.5 -- 0 sine -d %f -- %f %f 0 0 dc -d 0.5 -- 0' % \
                (duration, amplitude, f)
            cmd += ' | lcg stimulus -F %g -I %s -O %s --rt %s' % \
                (sampling_rate, ai, ao, realtime)
            sub.call(cmd, shell=True)

if __name__ == '__main__':
    main()


