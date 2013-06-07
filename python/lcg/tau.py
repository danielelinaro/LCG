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
    print('     -a    Amplitude of the stimulation (default -300 pA).')
    print('     -d    Duration of the stimulation (default 10 ms).')
    print('     -n    Number of repetitions (default 30).')
    print('     -i    Interval between repetitions (default 0.5 sec).')
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

    add_amplitude = True
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
                add_amplitude = False
            elif o == '-d':
                add_duration = False
            elif o == '-n':
                add_reps = False
            elif o == '-i':
                add_interval = False

    if add_amplitude: cmd += ' -a -300'
    if add_duration: cmd += ' -d 10'
    if add_reps: cmd += ' -n 30'
    if add_interval: cmd += ' -i 0.5'

    sub.call(cmd, shell=True)

if __name__ == '__main__':
    main()


