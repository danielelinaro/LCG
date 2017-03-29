#!/usr/bin/env python

import os
import sys
import getopt
import numpy as np
import subprocess as sub
import lcg

def usage():
    print('')
    print('   %s - Apply extracellular stimulation while holding the cell at different membrane potentials.' \
              % os.path.basename(sys.argv[0]))
    print('')
    print('Usage: %s [option <value>]' % os.path.basename(sys.argv[0]))
    print('')
    print('where options are:')
    print('             -h    display this help message and exit.')
    print('             -D    delay of the stimulation after application of the holding voltage (default 0.1 s).')
    print('             -H    baseline holding voltage (default -70 mV).')
    print('             -O    output channels (default %s,2).' % os.environ['AO_CHANNEL_VC'])
    print('             -I    input channel (default %s).' % os.environ['AI_CHANNEL_VC'])
    print('             -F    sampling frequency (default %s Hz).' % os.environ['SAMPLING_RATE'])
    print('             -d    stimulation duration (default 0.5 s).')
    print('             -n    number of repetitions of the stimulation (default 50).')
    print('             -i    interval between repetitions (default 2 s).')
    print('             -b    time before the beginning of the stimulation (default 0.2 s).')
    print('             -a    time after the end of the stimulation (default 0.5 s).')
    print('         --full    use Vhold = -80:10:40 instead of just Vhold = [-70,40].')
    print('   --input-gain    input conversion factor (default %s).' % os.environ['AI_CONVERSION_FACTOR_VC'])
    print('  --output-gain    output conversion factor (default %s)' % os.environ['AO_CONVERSION_FACTOR_VC'])
    print('')

def main():
    try:
        opts,args = getopt.getopt(sys.argv[1:], 'hH:n:F:d:i:b:a:D:I:O:',['help','input-gain=','output-gain=','full'])
    except getopt.GetoptError, err:
        print str(err)
        usage()
        sys.exit(1)
        
    V0 = -70.
    dur = 0.5
    pulseDuration = 1e-4
    pre = 0.2
    post = 0.5
    stimulusDelay = 0.1
    nTrials = 50
    interTrialInterval = 2.
    full = False
    samplingRate = float(os.environ['SAMPLING_RATE'])  # [Hz]
    analogOutputs = [int(os.environ['AO_CHANNEL_VC']),2]
    analogInput = int(os.environ['AI_CHANNEL_VC'])
    inputGain = float(os.environ['AI_CONVERSION_FACTOR_VC'])
    outputGain = float(os.environ['AO_CONVERSION_FACTOR_VC'])

    for o,a in opts:
        if o in ('-h','--help'):
            usage()
            sys.exit(0)
        elif o == '-n':
            nTrials = int(a)
        elif o == '-F':
            samplingRate = float(a)
        elif o == '-d':
            dur = float(a)
        elif o == '-i':
            interTrialInterval = float(a)
        elif o == '-b':
            pre = float(a)
        elif o == '-a':
            post = float(a)
        elif o == '-D':
            stimulusDelay = float(a)
        elif o == '-H':
            V0 = float(a)
        elif o == '--full':
            full = True
        elif o == '-I':
            analogInput = int(a)
        elif o == '-O':
            analogOutputs = map(int,a.split(','))
        elif o == '--input-gain':
            inputGain = float(a)
        elif o == '--output-gain':
            outputGains = float(a)

    if len(analogOutputs) != 2:
        print('You must specify two analog outputs in the form 0,1.')
        sys.exit(0)

    if stimulusDelay >= dur:
        print('The delay of stimulus application must be smaller than the duration of the holding potential.')
        sys.exit(0)

    sub.call('lcg-stimgen -o pulse.stim dc -d %g 0 dc -d %g 5 dc -d %g 0' % \
                 (pre+stimulusDelay,pulseDuration,dur-pulseDuration-stimulusDelay+post), shell=True)

    if full:
        Vhold = [-80,-70,-60,-50,-40,-30,-20,-10,0,10,20,30,40]
    else:
        Vhold = [-70,40]
    n = len(Vhold)

    for i,V in enumerate(Vhold):
        cmd = 'lcg-stimgen -o hold.stim dc -d %g -- %g dc -d %g -- %g dc -d %g -- %g' % (pre,V0,dur,V,post,V0)
        sub.call(cmd, shell=True)
        cmd = 'lcg-stimulus -s hold.stim,pulse.stim -n %d -i %g -O %s,%s --output-gain %s,1 --output-units mV,V ' + \
            '-I %s --input-gain %s --input-units pA'
        cmd = cmd % (nTrials,interTrialInterval,analogOutputs[0],analogOutputs[1],outputGain,analogInput,inputGain)
        sub.call(cmd, shell=True)
        if i < n-1:
            sub.call('sleep %g' % interTrialInterval, shell=True)
    
