#!/usr/bin/env python

import os
import sys
import getopt
import subprocess as sub
import lcg

configFile = 'pulses.xml'
stimFile = 'pulses.stim'

def usage():
    print('')
    print('   %s - Inject a train of brief pulses of current in a cell to test connectivity.' % os.path.basename(sys.argv[0]))
    print('\nUsage: %s [option <value>]' % os.path.basename(sys.argv[0]))
    print('\nwhere options are:\n')
    print('            -h    display this help message and exit.')
    print('            -f    frequency of the stimulation.')
    print('            -O    output channel.')
    print('            -I    input channels (in the form 0,1 where 0 and 1 are the channels corresponding to the pre- and post-synaptic cell).')
    print('            -n    number of pulses in the train (default 10).')
    print('            -F    sampling frequency (default %s Hz).' % os.environ['SAMPLING_RATE'])
    print('            -d    stimulation duration (default 1 ms).')
    print('            -A    stimulation amplitude.')
    print('            -N    number of repetitions of the stimulation (default 10).')
    print('            -i    interTrialInterval between repetitions (default 10 s).')
    print('            -b    time before the beginning of the stimulation (default 0.5 s).')
    print('            -a    time after the end of the stimulation (default 1 s).')
    print('            -p    duration of the pause between the last pulse in the train and the recovery one (default 0.5 s).')
    print('   --no-kernel    do not compute the computeKernel.')
    print(' --no-recovery    do not include a recovery pulse in the stimulation.')
    print('')

def main():
    try:
        opts,args = getopt.getopt(sys.argv[1:], 'hN:n:f:F:d:A:b:a:p:I:O:i:', ['help','no-kernel','no-recovery'])
    except getopt.GetoptError, err:
        print str(err)
        usage()
        sys.exit(1)

    stimulusFrequency = None        # [Hz]
    analogOutput = None
    analogInput = []
    stimulusAmplitude = None      # [pA]

    nTrials = 10
    nPulses = 10
    samplingRate = float(os.environ['SAMPLING_RATE'])  # [Hz]
    stimulusDuration = 1         # [ms]
    interTrialInterval = 10       # [s]
    computeKernel = True
    withRecovery = True
    pre = 0.5           # [s]
    post = 1            # [s]
    pause = 0.5         # [s]

    for o,a in opts:
        if o in ('-h','--help'):
            usage()
            sys.exit(0)
        elif o == '-N':
            nTrials = int(a)
        elif o == '-n':
            nPulses = int(a)
        elif o == '-f':
            stimulusFrequency = float(a)
        elif o == '-F':
            samplingRate = float(a)
        elif o == '-d':
            stimulusDuration = float(a)
        elif o == '-A':
            stimulusAmplitude = float(a)
        elif o == '-i':
            interTrialInterval = float(a)
        elif o == '-b':
            pre = float(a)
        elif o == '-a':
            post = float(a)
        elif o == '-p':
            pause = float(a)
        elif o == '-I':
            analogInput = []
            for ch in a.split(','):
                analogInput.append(int(ch))
        elif o == '-O':
            analogOutput = int(a)
        elif o == '--no-kernel':
            computeKernel = False
        elif o == '--no-recovery':
            withRecovery = False

    if stimulusFrequency == None:
        print('You must specify the stimulation frequency (-f switch).')
        sys.exit(1)

    if len(analogInput) != 2:
        print('You must specify the input channels (-I switch).')
        sys.exit(1)

    if analogOutput == None:
        print('You must specify the output channel (-O switch).')
        sys.exit(1)

    if stimulusAmplitude is None:
        print('You must specify the amplitude of the stimulation (-A switch)')
        sys.exit(1)

    if interTrialInterval <= 0:
        print('The interTrialInterval between repetitions must be positive.')
        sys.exit(1)

    if pre <= 0:
        print('The interTrialInterval before the beginning of the stimulation must be positive.')
        sys.exit(1)

    if post <= 0:
        print('The interTrialInterval after the end of the stimulation must be positive.')
        sys.exit(1)

    if withRecovery and pause <= 0:
        print('The interTrialInterval between pulse train and recovery pulse must positive.')
        sys.exit(1)

    if computeKernel:
        sub.call('lcg computeKernel -a -F ' + str(samplingRate) + ' -I ' + str(analogInput[0]) + ' -O ' + str(analogOutput) + ' --non-rt', shell=True)

    stim = [[pre,1,0,0,0,0,0,0,0,0,0,1],
            [nPulses/stimulusFrequency,8,stimulusAmplitude,-stimulusFrequency,stimulusDuration,0,0,0,0,0,0,1]]
    if withRecovery:
        stim.append([pause-(1./stimulusFrequency-stimulusDuration*1e-3),1,0,0,0,0,0,0,0,0,0,1])
        stim.append([stimulusDuration*1e-3,1,stimulusAmplitude,0,0,0,0,0,0,0,0,1])
    stim.append([post,1,0,0,0,0,0,0,0,0,0,1])
    totalDuration = lcg.writeStimFile(stimFile, stim, False)
    channels = [{'type':'input', 'channel':analogInput[0]},{'type':'input', 'channel':analogInput[1]},
                {'type':'output', 'channel':analogOutput, 'stimfile':stimFile}]
    lcg.writeIOConfigurationFile(configFile,samplingRate,totalDuration,channels)

    for i in range(nTrials):
        sub.call(lcg.common.prog_name + ' -c ' + configFile,shell=True)
        if i < nTrials-1:
            sub.call('sleep',shell=True)

if __name__ == '__main__':
    main()
