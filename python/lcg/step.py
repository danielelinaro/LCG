#!/usr/bin/env python

import os
import sys
import numpy as np
import getopt
import subprocess as sub
import lcg

stimuli_directory = 'stimuli'

def usage():
    print('This script injects a constant step of current into a cell.')
    print('')
    print('Usage: %s [option <value>]' % os.path.basename(sys.argv[0]))
    print('')
    print('where options are:')
    print('')
    print('              -a   stimulation amplitudes in the form start,[stop,step] (in pA).')
    print('              -d   stimulation duration (in seconds).')
    print('              -t   tail duration (0 pA of output after the stimulation, default 1 s)')
    print('              -n   number of repetitions of each amplitude (default 1)')
    print('              -i   interval between repetitions (default 1 s)')
    print('              -I   input channel (default %s)' % os.environ['AI_CHANNEL'])
    print('              -O   output channel (default %s)' % os.environ['AO_CHANNEL'])
    print('              -F   sampling frequency (default %s Hz)' % os.environ['SAMPLING_RATE'])
    print('              -H   holding current (default 0 pA)')
    print('            --rt   use real-time system (yes or no, default %s)' % os.environ['LCG_REALTIME'])
    print(' --with-preamble   include stability preamble.')
    print('    --no-shuffle   do not shuffle trials.')
    print('     --no-kernel   do not compute the electrode kernel.')
    print('         --model   use a leaky integrate-and-fire model.')
    print('')

def main():
    try:
        opts,args = getopt.getopt(sys.argv[1:],
                                  'hd:a:t:n:i:I:O:F:H:',
                                  ['help','with-preamble','no-preamble',
                                   'no-shuffle','no-kernel',
                                   'model','rt='])
    except getopt.GetoptError, err:
        print(str(err))
        usage()
        sys.exit(1)

    suffix = 'CC'
    ao = int(os.environ['AO_CHANNEL_{0}'.format(suffix)])
    ai = int(os.environ['AI_CHANNEL_{0}'.format(suffix)])
    samplf = float(os.environ['SAMPLING_RATE'])    # [Hz]
    holding = 0    # [pA]
    with_preamble = False
    shuffle = True
    kernel = True
    nreps = 1
    duration = None    # [s]
    interval = 1       # [s]
    tail = 1           # [s]
    stim_ampl = []     # [pA]
    realtime = os.environ['LCG_REALTIME']
    model = ''
    for o,a in opts:
        if o in ['-h','--help']:
            usage()
            sys.exit(0)
        elif o == '-d':
            duration = float(a)
        elif o == '-a':
            for amp in a.split(','):
                stim_ampl.append(float(amp))
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
        elif o == '-H':
            holding = float(a)
        elif o == '--with-preamble':
            with_preamble = True
        elif o == '--no-shuffle':
            shuffle = False
        elif o == '--no-kernel':
            kernel = False
        elif o == '--model':
            model = '--model'
            realtime = 'yes'
            kernel = False
        elif o == '--rt':
            realtime = a

    if duration is None:
        print('You must specify the duration of the stimulation (-d switch)')
        sys.exit(1)

    if len(stim_ampl) == 1:
        stim_ampl.append(stim_ampl[0])
        stim_ampl.append(1)
    elif len(stim_ampl) != 3:
        print('The amplitudes must be in the form start[,stop,step].')
        sys.exit(1)

    amplitudes = np.arange(stim_ampl[0],stim_ampl[1]+1,stim_ampl[2])
    if shuffle:
        np.random.shuffle(amplitudes)

    try:
        os.mkdir(stimuli_directory,0755)
    except:
        ok = raw_input('The directory ['+stimuli_directory+'] already exists: shall I continue (deleting its content)? [y/N] ')
        if ok != 'y':
            sys.exit(0)
        for f in os.listdir(stimuli_directory):
            try:
                if os.path.isfile(os.path.join(stimuli_directory,f)):
                    os.unlink(os.path.join(stimuli_directory,f))
            except e:
                print e
                sys.exit(1)

    if with_preamble:
        stimulus = [[duration,1,holding,0,0,0,0,0,0,0,0,1],
                    [tail,1,holding,0,0,0,0,0,0,0,0,1]]
        row = 0
    else:
        stimulus = [[tail,1,holding,0,0,0,0,0,0,0,0,1],
                    [duration,1,holding,0,0,0,0,0,0,0,0,1],
                    [tail,1,holding,0,0,0,0,0,0,0,0,1]]
        row = 1

    for i,amp in enumerate(amplitudes):
        stimulus[row][2] = amp + holding
        lcg.writeStimFile('%s/step_%02d.stim' % (stimuli_directory,i+1), stimulus, 
                          with_preamble, preamble_holding=holding)

    if kernel:
        sub.call('lcg kernel -I %d -O %d -F %g -H %g --rt %s' % (ai,ao,samplf,holding,realtime), shell=True)
    
    sub.call('lcg stimulus -d %s -i %g -I %d -O %d -n %d -F %g --rt %s %s' % 
             (stimuli_directory,
              interval,ai,ao,
              nreps,samplf,
              realtime,
              model), shell=True)

if __name__ == '__main__':
    main()


