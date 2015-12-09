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
    print('   %s - Apply a train of brief extracellular or intracellular pulses.' % os.path.basename(sys.argv[0]))
    print('')
    print('Usage: %s [option <value>]' % os.path.basename(sys.argv[0]))
    print('')
    print('where options are:')
    print('             -h    display this help message and exit.')
    print('             -f    frequency of the stimulation.')
    print('             -O    output channel.')
    print('             -A    stimulation amplitude.')
    print('             -I    input channel (default %s for current clamp, %s for voltage clamp).' \
              % (os.environ['AI_CHANNEL_CC'],os.environ['AI_CHANNEL_VC']))
    print('             -N    number of pulses in the train (default 10).')
    print('             -F    sampling frequency (default %s Hz).' % os.environ['SAMPLING_RATE'])
    print('             -d    stimulation duration (default 1 ms).')
    print('             -n    number of repetitions of the stimulation (default 10).')
    print('             -i    interval between repetitions (default 10 s).')
    print('             -b    time before the beginning of the stimulation (default 0.5 s).')
    print('             -a    time after the end of the stimulation (default 1 s).')
    print('             -p    duration of the pause between the last pulse in the train and the recovery one (default 0.5 s).')
    print('  --with-kernel    compute the kernel.')
    print('--with-recovery    include a recovery pulse in the stimulation.')
    print('   --input-gain    input conversion factor (default %s for current clamp, %s for voltage clamp).' \
              % (os.environ['AI_CONVERSION_FACTOR_CC'],os.environ['AI_CONVERSION_FACTOR_VC']))
    print('  --output-gain    output conversion factor (default 1 or %s if --intra is specified).' \
              % os.environ['AO_CONVERSION_FACTOR_CC'])
    print('  --input-units    input units (default %s for current clamp, %s for voltage clamp).' \
              % (os.environ['AI_UNITS_CC'],os.environ['AI_UNITS_VC']))
    print(' --output-units    output units (default V or %s if --intra is specified).' \
              % os.environ['AO_UNITS_CC'])
    print('       --vclamp    record the cell in voltage clamp mode.')
    print('        --intra    perform intracellular stimulation.')
    print('')

def main():
    try:
        opts,args = getopt.getopt(sys.argv[1:], 'hN:n:f:F:d:A:b:a:p:I:O:i:',
                                  ['help','with-kernel','with-recovery','vclamp',
                                   'input-gain=','output-gain=','input-units=','output-units=','intra'])
    except getopt.GetoptError, err:
        print str(err)
        usage()
        sys.exit(1)

    stimulusFrequency = None        # [Hz]
    analogOutput = None
    analogInput = int(os.environ['AI_CHANNEL_CC'])
    stimulusAmplitude = None      # [V]

    nTrials = 10
    nPulses = 10
    samplingRate = float(os.environ['SAMPLING_RATE'])  # [Hz]
    stimulusDuration = 1         # [ms]
    interTrialInterval = 10       # [s]
    computeKernel = False
    withRecovery = False
    pre = 0.5           # [s]
    post = 1            # [s]
    pause = 0.5         # [s]
    vclamp = False

    inputGain = None
    outputGain = None
    inputUnits = None
    outputUnits = None

    intracellular = False

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
            analogInput = int(a)
        elif o == '-O':
            analogOutput = int(a)
        elif o == '--with-kernel':
            computeKernel = True
        elif o == '--with-recovery':
            withRecovery = True
        elif o == '--vclamp':
            vclamp = True
        elif o == '--input-gain':
            inputGain = float(a)
        elif o == '--output-gain':
            outputGain = float(a)
        elif o == '--input-units':
            inputUnits = a
        elif o == '--output-units':
            outputUnits = a
        elif o == '--intra':
            intracellular = True
            computeKernel = True

    if intracellular and vclamp:
        print('You cannot specify --intra and --vclamp at the same time.')
        sys.exit(1)

    if inputGain is None:
        if vclamp:
            inputGain = float(os.environ['AI_CONVERSION_FACTOR_VC'])
            inputUnits = os.environ['AI_UNITS_VC']
        else:
            inputGain = float(os.environ['AI_CONVERSION_FACTOR_CC'])
            inputUnits = os.environ['AI_UNITS_CC']

    if intracellular and analogOutput is None:
        analogOutput = int(os.environ['AO_CHANNEL_CC'])

    if outputGain is None:
        if intracellular:
            outputGain = float(os.environ['AO_CONVERSION_FACTOR_CC'])
            outputUnits = os.environ['AO_UNITS_CC']
        else:
            outputGain = 1.0
            outputUnits = 'V'

    if stimulusFrequency is None:
        print('You must specify the stimulation frequency (-f switch).')
        sys.exit(1)

    if analogOutput is None:
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
        sub.call('lcg kernel -F ' + str(samplingRate) + ' -I ' + str(analogInput) + ' -O ' + str(analogOutput) + \
                     ' --input-factor ' + str(inputGain) + ' --output-factor ' + str(outputGain) + \
                     ' --input-units ' + inputUnits + ' --output-units ' + outputUnits, shell=True)

    stim = [[pre,1,0,0,0,0,0,0,0,0,0,1],
            [nPulses/stimulusFrequency,8,stimulusAmplitude,-stimulusFrequency,stimulusDuration,0,0,0,0,0,0,1]]
    if withRecovery:
        stim.append([pause-1./stimulusFrequency,1,0,0,0,0,0,0,0,0,0,1])
        stim.append([stimulusDuration*1e-3,1,stimulusAmplitude,0,0,0,0,0,0,0,0,1])
    stim.append([post,1,0,0,0,0,0,0,0,0,0,1])
    totalDuration = lcg.writeStimFile(stimFile, stim, False)
    channels = [{'type':'input', 'channel':analogInput, 'factor':inputGain, 'units':inputUnits},
                {'type':'output', 'channel':analogOutput, 'factor':outputGain, 'units':outputUnits, 'stimfile':stimFile}]
    lcg.writeIOConfigurationFile(configFile,samplingRate,totalDuration,channels)

    for i in range(nTrials):
        sub.call(lcg.common.prog_name + ' -c ' + configFile, shell=True)
        if i < nTrials-1:
            sub.call('sleep ' + str(interTrialInterval), shell=True)

if __name__ == '__main__':
    main()
