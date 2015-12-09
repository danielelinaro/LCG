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
    print('   %s - Inject a train of brief pulses of current in a cell.' % os.path.basename(sys.argv[0]))
    print('')
    print('This script can be used to test the connectivity between two cells by injecting a')
    print('train of brief, strong pulses into a (presynaptic) cell to elicit action potentials')
    print('while recording the response into a second (postsynaptic) cell.')
    print('Note that, by default, the presynaptic cell is recorded in current clamp mode,')
    print('while the postsynaptic one is recorded in voltage clamp. It is up to the experimenter')
    print('to configure the amplifier appropriately.')
    print('')
    print('Usage: %s [option <value>]' % os.path.basename(sys.argv[0]))
    print('')
    print('where options are:')
    print('             -h    display this help message and exit.')
    print('             -f    frequency of the stimulation.')
    print('             -O    output channel.')
    print('             -I    input channels (in the form 0,1 where 0 and 1 are the channels corresponding to the pre- and post-synaptic cell).')
    print('             -N    number of pulses in the train (default 10).')
    print('             -F    sampling frequency (default %s Hz).' % os.environ['SAMPLING_RATE'])
    print('             -d    stimulation duration (default 1 ms).')
    print('             -A    stimulation amplitude.')
    print('             -n    number of repetitions of the stimulation (default 10).')
    print('             -i    interval between repetitions (default 10 s).')
    print('             -b    time before the beginning of the stimulation (default 0.5 s).')
    print('             -a    time after the end of the stimulation (default 1 s).')
    print('             -p    duration of the pause between the last pulse in the train and the recovery one (default 0.5 s).')
    print('    --no-kernel    do not compute the kernel.')
    print('  --no-recovery    do not include a recovery pulse in the stimulation.')
    print('  --input-gains    input conversion factors (default %s,%s).' % (os.environ['AI_CONVERSION_FACTOR_CC'],os.environ['AI_CONVERSION_FACTOR_VC']))
    print('  --output-gain    output conversion factor (default %s).' % os.environ['AO_CONVERSION_FACTOR_CC'])
    print('  --input-units    input units (default %s,%s).' % (os.environ['AI_UNITS_CC'],os.environ['AI_UNITS_VC']))
    print(' --output-units    output units (default %s).' % os.environ['AO_UNITS_CC'])
    print('       --vclamp    record the post-synaptic cell in voltage clamp mode (default).')
    print('       --cclamp    record the post-synaptic cell in current clamp mode.')
    print('')

def main():
    try:
        opts,args = getopt.getopt(sys.argv[1:], 'hN:n:f:F:d:A:b:a:p:I:O:i:',
                                  ['help','no-kernel','no-recovery','vclamp','cclamp',
                                   'input-gains=','output-gain=','input-units=','output-units='])
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
    vclamp = False
    cclamp = False

    inputGains = []
    outputGain = float(os.environ['AO_CONVERSION_FACTOR_CC'])
    inputUnits = []
    outputUnits = os.environ['AO_UNITS_CC']

    for o,a in opts:
        if o in ('-h','--help'):
            usage()
            sys.exit(0)
        elif o == '-n':
            nTrials = int(a)
        elif o == '-N':
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
        elif o == '--vclamp':
            vclamp = True
        elif o == '--cclamp':
            cclamp = True
        elif o == '--input-gains':
            for gain in a.split(','):
                inputGains.append(float(gain))
        elif o == '--output-gain':
            outputGain = float(a)
        elif o == '--input-units':
            for units in a.split(','):
                inputUnits.append(units)
        elif o == '--output-units':
            outputUnits = a
                
    if vclamp and cclamp:
        print('You must specify either one of --vclamp or --cclamp.')
        sys.exit(1)

    if not vclamp and not cclamp:
        vclamp = True

    if len(inputGains) == 0:
        inputGains.append(float(os.environ['AI_CONVERSION_FACTOR_CC']))
        if vclamp:
            inputGains.append(float(os.environ['AI_CONVERSION_FACTOR_VC']))
        else:
            inputGains.append(float(os.environ['AI_CONVERSION_FACTOR_CC']))

    if len(inputGains) != 2:
        print('The input conversion factors must be two, specified as in \'--input-factors %s,%s\'.' %
              (os.environ['AI_CONVERSION_FACTOR_CC'],os.environ['AI_CONVERSION_FACTOR_VC']))
        sys.exit(1)

    if len(inputUnits) == 0:
        inputUnits.append(os.environ['AI_UNITS_CC'])
        if vclamp:
            inputUnits.append(os.environ['AI_UNITS_VC'])
        else:
            inputUnits.append(os.environ['AI_UNITS_CC'])

    if len(inputUnits) != 2:
        print('The input units must be two, specified as in \'--input-units %s,%s\'.' %
              (os.environ['AI_UNITS_CC'],os.environ['AI_UNITS_VC']))
        sys.exit(1)

    if stimulusFrequency == None:
        print('You must specify the stimulation frequency (-f switch).')
        sys.exit(1)

    if len(analogInput) != 2:
        print('You must specify the input channels (-I switch).')
        sys.exit(1)

    if analogInput[0] == analogInput[1]:
        print('The input channels must be distinct (e.g., \'-I 0,1\').')
        sys.exit(1)

    if analogOutput == None:
        print('You must specify the output channel (-O switch).')
        sys.exit(1)

    if stimulusAmplitude is None:
        print('You must specify the amplitude of the stimulation (-A switch)')
        sys.exit(1)

    if interTrialInterval <= 0:
        print('The interval between repetitions must be positive.')
        sys.exit(1)

    if pre <= 0:
        print('The interval before the beginning of the stimulation must be positive.')
        sys.exit(1)

    if post <= 0:
        print('The interval after the end of the stimulation must be positive.')
        sys.exit(1)

    if withRecovery and pause <= 0:
        print('The interval between pulse train and recovery pulse must positive.')
        sys.exit(1)

    if computeKernel:
        sub.call('lcg kernel -F ' + str(samplingRate) + ' -I ' + str(analogInput[0]) + ' -O ' + str(analogOutput) + \
                     ' --input-factor ' + str(inputGains[0]) + ' --output-factor ' + str(outputGain) + \
                     ' --input-units ' + inputUnits[0] + ' --output-units ' + outputUnits + ' --non-rt', shell=True)

    stim = [[pre,1,0,0,0,0,0,0,0,0,0,1],
            [nPulses/stimulusFrequency,8,stimulusAmplitude,-stimulusFrequency,stimulusDuration,0,0,0,0,0,0,1]]
    if withRecovery:
        stim.append([pause-1./stimulusFrequency,1,0,0,0,0,0,0,0,0,0,1])
        stim.append([stimulusDuration*1e-3,1,stimulusAmplitude,0,0,0,0,0,0,0,0,1])
    stim.append([post,1,0,0,0,0,0,0,0,0,0,1])
    totalDuration = lcg.writeStimFile(stimFile, stim, False)
    channels = [{'type':'input', 'channel':analogInput[0], 'factor':inputGains[0], 'units':inputUnits[0]},
                {'type':'input', 'channel':analogInput[1], 'factor':inputGains[1], 'units':inputUnits[1]},
                {'type':'output', 'channel':analogOutput, 'factor':outputGain, 'units':outputUnits, 'stimfile':stimFile}]
    lcg.writeIOConfigurationFile(configFile,samplingRate,totalDuration,channels)

    for i in range(nTrials):
        sub.call(lcg.common.prog_name + ' -c ' + configFile, shell=True)
        if i < nTrials-1:
            sub.call('sleep ' + str(interTrialInterval), shell=True)

if __name__ == '__main__':
    main()
