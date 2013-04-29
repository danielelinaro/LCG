#!/usr/bin/env python

import os
import sys
import getopt
import lcg

stim_file = 'pulses.stim'

def usage():
    print('\nUsage: %s [option <value>]' % os.path.basename(sys.argv[0]))
    print('\nwhere options are:\n')
    print('     -a    pulse amplitude (default -100 pA)')
    print('     -d    pulse duration (default 200 ms)')
    print('     -f    pulse frequency (default 1 Hz)')
    print('     -D    total duration (default 60 s)')
    print('     -I    input channel (default 0)')
    print('     -O    output channel (default 0)')
    print('     -F    sampling frequency (default 20000 Hz)')

    print('\nThe option --poisson allows to specify that the intervals between')
    print('pulses should be distributed as a Poisson distribution.')
    print('')

def main():
    try:
        opts,args = getopt.getopt(sys.argv[1:], 'hd:a:f:D:I:O:F:', ['help','poisson'])
    except getopt.GetoptError, err:
        print(str(err))
        usage()
        sys.exit(1)

    pulse_amplitude = -100  # [pA]
    pulse_duration = 200    # [ms]
    pulse_frequency = 1     # [Hz]
    duration = 60           # [s]
    ai = 0
    ao = 0
    srate = 20000
    poisson = False

    for o,a in opts:
        if o in ['-h','--help']:
            usage()
            sys.exit(0)
        elif o == '-d':
            pulse_duration = float(a)
        elif o == '-a':
            pulse_amplitude = float(a)
        elif o == '-f':
            pulse_frequency = float(a)
        elif o == '-D':
            duration = float(a)
        elif o == '-I':
            ai = int(a)
        elif o == '-O':
            ao = int(a)
        elif o == '-F':
            srate = int(a)
        elif o == '--poisson':
            poisson = True
    
    if not poisson:
        pulse_frequency = -pulse_frequency

    stimulus = [[1,1,0,0,0,0,0,0,5061983,0,0,1],
                [duration,8,pulse_amplitude,pulse_frequency,pulse_duration,0,0,0,5061983,0,0,1]]

    os.system('kernel_protocol -I ' + str(ai) + ' -O ' + str(ao) + ' -F '+ str(srate))
    lcg.writeStimFile(stim_file, stimulus, False)
    os.system('cclamp -f ' + stim_file + ' -F '+ str(srate))

if __name__ == '__main__':
    main()


