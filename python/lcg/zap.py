#!/usr/bin/env python

import os
import sys
import numpy as np
import getopt
import subprocess as sub
import lcg

stim_file = 'zap.stim'

def usage():
    print('This script injects a chirp of current into a cell.')
    print('')
    print('Usage: %s [option <value>]' % os.path.basename(sys.argv[0]))
    print('')
    print('where options are:')
    print('')
    print('          -f   initial and final frequencies (comma separated values, default 0.1,200).')
    print('          -a   stimulation amplitude (default 50 pA).')
    print('          -o   offset (default 0 pA).')
    print('          -d   stimulation duration (default 12 sec).')
    print('          -t   tail duration (0 pA of output after the stimulation, default 1 sec).')
    print('          -n   number of repetitions (default 20).')
    print('          -i   interval between repetitions (default 1 sec).')
    print('          -I   input channel (default %s).' % os.environ['AI_CHANNEL'])
    print('          -O   output channel (default %s).' % os.environ['AO_CHANNEL'])
    print('          -F   sampling frequency (default %s Hz).' % os.environ['SAMPLING_RATE'])
    print('        --rt   use real-time system (yes or no, default %s).' % os.environ['LCG_REALTIME'])
    print('  --preamble   include stability preamble (yes or no, default yes).')
    print('    --kernel   compute the electrode kernel (yes or no, default yes).')
    print('')

def main():
    try:
        opts,args = getopt.getopt(sys.argv[1:], 'hf:a:o:d:t:n:i:I:O:F:',
                                  ['help','preamble=','kernel=','rt='])
    except getopt.GetoptError, err:
        print(str(err))
        usage()
        sys.exit(1)

    freq = [0.1,200.]
    ao = int(os.environ['AO_CHANNEL'])
    ai = int(os.environ['AI_CHANNEL'])
    samplf = float(os.environ['SAMPLING_RATE'])    # [Hz]
    preamble = True
    kernel = True
    nreps = 20
    duration = 12      # [s]
    interval = 1       # [s]
    tail = 1           # [s]
    amplitude = 50     # [pA]
    offset = 0         # [pA]
    realtime = os.environ['LCG_REALTIME']

    for o,a in opts:
        if o in ['-h','--help']:
            usage()
            sys.exit(0)
        elif o == '-f':
            for i,f in enumerate(a.split(',')):
                freq[i] = float(f)
                if freq[i] <= 0:
                    print('All frequencies must be positive.')
                    sys.exit(0)
            if i != 1:
                print('You must provide both initial and final frequency with the -f switch, as in -f 1,20.')
                sys.exit(0)
        elif o == '-d':
            duration = float(a)
        elif o == '-a':
            amplitude = float(a)
        elif o == '-o':
            offset = float(a)
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
        elif o == '-F':
            samplf = float(a)
        elif o == '--preamble':
            if not a in ('yes','no'):
                print('--preamble accepts either `yes\' or `no\'.')
                sys.exit(1)
            preamble = a == 'yes'
        elif o == '--kernel':
            if not a in ('yes','no'):
                print('--kernel accepts either `yes\' or `no\'.')
                sys.exit(1)
            kernel = a == 'yes'
        elif o == '--rt':
            if not a in ('yes','no'):
                print('--rt accepts either `yes\' or `no\'.')
                sys.exit(1)
            realtime = a

    if duration <= 0:
        print('The duration of the stimulation must be positive.')
        sys.exit(1)

    stimulus = 'lcg-stimgen -o %s ' % stim_file
    if preamble:
        stimulus += 'dc -d 0.5 0 dc -d 0.01 -- -300 dc -d 0.5 0 dc -d 0.6 -- -100 '
    stimulus += 'dc -d 1 0 dc -d %g -p -- %g chirp -E -- %g %g %g 1 dc -d %g 0' % (duration,offset,amplitude,freq[0],freq[1],tail)

    sub.call(stimulus, shell=True)

    if kernel:
        sub.call('lcg-kernel -I %d -O %d -F %g --rt %s' % (ai,ao,samplf,realtime), shell=True)
    
    sub.call('lcg-stimulus -s %s -i %g -I %d -O %d -n %d -F %g --rt %s' % (stim_file,interval,ai,ao,nreps,samplf,realtime), shell=True)

if __name__ == '__main__':
    main()


