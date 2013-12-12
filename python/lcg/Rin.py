#!/usr/bin/env python

import os
import sys
import getopt
import subprocess as sub
import lcg

stim_file = 'pulses.stim'

def usage():
    print('')
    print('This program injects a series of hyperpolarizing pulses of current into')
    print('a cell to compute its input resistance. The frequency of the pulses is low')
    print('and it is therefore suited for application to in vivo experiments.')
    print('')
    print('Usage: %s [option <value>]' % os.path.basename(sys.argv[0]))
    print('')
    print('where options are:')
    print('')
    print('         -a   pulse amplitude (default -100 pA)')
    print('         -d   pulse duration (default 200 ms)')
    print('         -f   pulse frequency (default 1 Hz)')
    print('         -D   total duration (default 60 s)')
    print('         -N   number of pulses (in alternative to the -D option)')
    print('         -I   input channel (default %s)' % os.environ['AI_CHANNEL'])
    print('         -O   output channel (default %s)' % os.environ['AO_CHANNEL'])
    print('         -F   sampling frequency (default %s Hz)' % os.environ['SAMPLING_RATE'])
    print('  --poisson   the intervals between stimulations will have a Poisson distribution.')
    print('')

def main():
    try:
        opts,args = getopt.getopt(sys.argv[1:], 'hd:a:f:D:N:I:O:F:', ['help','poisson'])
    except getopt.GetoptError, err:
        print(str(err))
        usage()
        sys.exit(1)

    pulse_amplitude = -100  # [pA]
    pulse_duration = 200    # [ms]
    pulse_frequency = 1     # [Hz]
    duration = None         # [s]
    npulses = None           
    ai = int(os.environ['AI_CHANNEL'])
    ao = int(os.environ['AO_CHANNEL'])
    srate = float(os.environ['SAMPLING_RATE'])
    poisson = False

    for o,a in opts:
        if o in ['-h','--help']:
            usage()
            sys.exit(0)
        elif o == '-d':
            pulse_duration = float(a)
            if pulse_duration <= 0:
                print('The duration of the current pulses must be positive.')
                sys.exit(1)
        elif o == '-a':
            pulse_amplitude = float(a)
        elif o == '-f':
            pulse_frequency = float(a)
            if pulse_frequency <= 0:
                print('The frequency of the stimulation must be positive.')
                sys.exit(1)
        elif o == '-D':
            duration = float(a)
            if duration <= 0:
                print('The duration of the stimulation must be positive.')
                sys.exit(1)
        elif o == '-N':
            npulses = int(a)
            if npulses <= 0:
                print('The number of pulses must be positive.')
                sys.exit(1)
        elif o == '-I':
            ai = int(a)
        elif o == '-O':
            ao = int(a)
        elif o == '-F':
            srate = int(a)
            if srate <= 0:
                print('The sampling frequency must be positive.')
                sys.exit(1)
        elif o == '--poisson':
            poisson = True
    
    if duration is None:
        if npulses is None:
            duration = 60
        else:
            duration = float(npulses)/pulse_frequency
    elif npulses is not None:
        print('Ignoring the -N option, since -D is provided.')
    
    if not poisson:
        pulse_frequency = -pulse_frequency

    stimulus = [[1,1,0,0,0,0,0,0,0,0,0,1],
                [duration,8,pulse_amplitude,pulse_frequency,pulse_duration,0,0,0,0,0,0,1],
                [1,1,0,0,0,0,0,0,0,0,0,1]]

    #sub.call('lcg kernel -I ' + str(ai) + ' -O ' + str(ao) + ' -F '+ str(srate), shell=True)
    lcg.writeStimFile(stim_file, stimulus, False)
    #sub.call('lcg vcclamp -f ' + stim_file + ' -F '+ str(srate), shell=True)

if __name__ == '__main__':
    main()


