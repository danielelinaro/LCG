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
    print('     -F    Sampling frequency (default %s Hz).' % os.environ['SAMPLING_RATE'])
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

    got_amplitude = False
    add_duration = True
    add_reps = True
    add_interval = True

    cmd = 'lcg pulse'
    for o,a in opts:
        if o in ('-h','--help'):
            usage()
            sys.exit(0)
        else:
            cmd += ' ' + o + ' ' + a
            if o == '-a':
                got_amplitude = True
            elif o == '-d':
                add_duration = False
            elif o == '-n':
                add_reps = False
            elif o == '-i':
                add_interval = False

    if not got_amplitude:
        print('You must specify the amplitude of the stimulation (-a switch).')
        sys.exit(1)

    if add_duration: cmd += ' -d 1'
    if add_reps: cmd += ' -n 20'
    if add_interval: cmd += ' -i 1'
 
    sub.call(cmd, shell=True)

if __name__ == '__main__':
    main()


