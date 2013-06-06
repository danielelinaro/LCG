#!/usr/bin/env python

import os
import sys
import getopt
import lcg
import subprocess as sub

def usage():
    print('\nThis script injects a single (brief) pulse of current. Its purpose is to')
    print('elicit a single spike to extract its salient electrophysiological properties.')
    print('The duration of the pulse should be short enough in order not to interfere')
    print('with the actual shape of the action potential.')
    print('\nUsage: %s [option <value>]' % os.path.basename(sys.argv[0]))
    print('\nwhere options are:\n')
    print('     -h    Display this help message and exit.')
    print('     -a    Amplitude of the stimulation (in pA).')
    print('     -d    Duration of the stimulation (default 1 ms).')
    print('     -n    Number of repetitions (default 20).')
    print('     -i    Interval between repetitions (default 1 sec).')
    print('     -F    Sampling frequency (default 20000 Hz)).')
    print('     -I    Input channel (default %s)' % os.environ['AI_CHANNEL'])
    print('     -O    Output channel (default %s)' % os.environ['AO_CHANNEL'])
    print('')

def main():
    try:
        opts,args = getopt.getopt(sys.argv[1:], 'ha:d:n:i:I:O:F:', ['help'])
    except getopt.GetoptError, err:
        print(str(err))
        usage()
        sys.exit(1)

    amplitude = None             # [pA]
    duration = 1e-3              # [s]
    reps = 20
    interval = 1                 # [s]
    sampling_frequency = 20000   # [Hz]
    ai = int(os.environ['AI_CHANNEL'])
    ao = int(os.environ['AO_CHANNEL'])

    for o,a in opts:
        if o in ('-h','--help'):
            usage()
            sys.exit(0)
        elif o == '-a':
            amplitude = float(a)
        elif o == '-d':
            duration = float(a)*1e-3
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

    if not amplitude:
        print('You must specify the amplitude of the stimulation (-a switch).')
        sys.exit(1)

    stim = [[0.5,1,0,0,0,0,0,0,0,0,0,1],
            [duration,1,amplitude,0,0,0,0,0,0,0,0,1],
            [0.5,1,0,0,0,0,0,0,0,0,0,1]]

    lcg.writeStimFile('ap.stim',stim,False)
    
    sub.call('lcg kernel -I ' + str(ai) + ' -O ' + str(ao) + ' -F ' + str(sampling_frequency), shell=True)
    sub.call('lcg vcclamp -f ap.stim -n ' + str(reps) + ' -i ' + str(interval) + ' -F ' + str(sampling_frequency), shell=True)

if __name__ == '__main__':
    main()


