#!/usr/bin/env python

import getopt
import glob
import lcg
import numpy as np
import os
import subprocess as sub
import sys
import time
from lcg.utils import runCommand
import random

config_file = 'stimulus.xml'

def usage():
    print('This program applies the stimulation described by one or more stimulus files')
    print('to the specified output channel(s), while at the same time recording from the')
    print('corresponding input channels, located on the same subdevice.')
    print('')
    print('Usage: %s [option <value>]' % os.path.basename(sys.argv[0]))
    print('')
    print('where options are:')
    print('')
    print(' -h, --help               display this help message and exit')
    print(' -s, --stimulus           stimulus file(s) to use')
    print(' -d, --directory          directory where the stimulus files are located')
    print(' -l, --duration           duration of the recording (only if -s or -d are not specified)')
    print(' -n, --repetitions        number of repetitions (default 1)')
    print(' -i, --interval           interval between repetitions (default 0 s)')
    print(' -o, --output-file        name of the file where data will be saved.')
    print(' -F, --sampling-rate      sampling frequency (default %s Hz)' % os.environ['SAMPLING_RATE'])
    print(' -D, --device             input device (default %s)' % os.environ['COMEDI_DEVICE'])
    print('     --input-subdevice    input subdevice (default %s)' % os.environ['AI_SUBDEVICE'])
    print('     --output-subdevice   output subdevice (default %s)' % os.environ['AO_SUBDEVICE'])
    print(' -I, --input-channels     input channels (default %s (%s in VC), but see note at the end for how to specify input channels)' % (os.environ['AI_CHANNEL_CC'],os.environ['AI_CHANNEL_VC']))
    print(' -g, --input-gains        input conversion factors (comma separated values, default %s' % os.environ['AI_CONVERSION_FACTOR_CC'])
    print('                          (or %s if --voltage-clamp is used) for all channels)' % os.environ['AI_CONVERSION_FACTOR_VC'])
    print(' -u, --input-units        input units (comma separated values, default %s (or %s if' % (os.environ['AI_UNITS_CC'],os.environ['AI_UNITS_VC']))
    print('                          --vclamp is used) for all channels)')
    print(' -O, --output-channels    output channel(s) (default %s (%s in VC), but see note at the end for how to specify output channels)' % (os.environ['AO_CHANNEL'],os.environ['AO_CHANNEL_VC']))
    print(' -G, --output-gains       output conversion factors (comma separated values, default %s' % os.environ['AO_CONVERSION_FACTOR_CC'])
    print('                          (or %s if --vclamp is used) for all channels)' % os.environ['AO_CONVERSION_FACTOR_VC'])
    print(' -U, --output-units       output units (comma separated values, default %s (or %s if' % (os.environ['AO_UNITS_CC'],os.environ['AO_UNITS_VC']))
    print('                          --vclamp is used) for all channels)')
    print(' -V, --voltage-clamp      use default conversion factor and units for voltage clamp')
    print(' -E, --conductance        reversal potentials for conductance clamp experiment (comma separated values (mV))')
    print(' -H, --offset             offset value, summed to the stimulation (in pA or mV, default 0)')
    print(' -R, --reset-output       whether output should be reset to 0 after every trial (yes or no,')
    print('                          default %s for current clamp and no for voltage clamp experiments)' % os.environ['LCG_RESET_OUTPUT'])
    print(' -p, --priority           set the priority of this thread, -20 is maximum, default is zero')
    print('     --shuffle            shuffle the order in which stimulus files are applied')
    print('     --rt                 use real-time engine (yes or no, default %s)' % os.environ['LCG_REALTIME'])
    print('     --verbose            set the verbosity level of lcg-experiment (default is 4 - silent)')
    print('                          it is also possible to specify options to control the behavior of')
    print('                          lcg-stimulus using "timer", "percent" or "silent"')
    print('     --model              use a LIF neuron instead of real experiment')
    print('     --dry-run            do not run, simply generate the commands')
    print('')
    print('Input and output channels (-I and -O switches, respectively) can be specified in one of four ways:')
    print('')
    print('  1) single value, as in `-I 1\'')
    print('  2) a list of comma separated values, as in `-I 0,1,2,3\'')
    print('  3) a range of values, as in `-I 0-3\', which is equivalent to `-I 0,1,2,3\'')
    print('  4) the string ``none\'\', as in `-I none\', in which case no input or output channels are used')
    print('')

def get_stimulus_duration(stimfile):
    stim = np.loadtxt(stimfile)
    if len(stim.shape) == 1:
        return stim[0]
    return np.sum(stim[:,0])

def main():
    try:
        opts,args = getopt.getopt(sys.argv[1:], 'hs:d:l:n:i:F:D:I:g:u:O:G:U:Vo:E:H:R:p:',
                                  ['help','stimulus=','directory=',
                                   'duration=','repetitions=','interval=','sampling-rate=',
                                   'device=','input-subdevice=','output-subdevice=',
                                   'input-channels=','input-gains=','input-units=',
                                   'output-channels=','output-gains=','output-units=',
                                   'voltage-clamp','offset=','conductance=','rt=','output-file=',
                                   'reset-output=','verbose=','dry-run','model','shuffle','priority='])
    except getopt.GetoptError, err:
        print(str(err))
        usage()
        sys.exit(1)

    # default values
    stimfiles = None
    stimdir = None
    duration = None
    repetitions = 1
    interval = 0       # [s]
    
    samplingRate = float(os.environ['SAMPLING_RATE'])    # [Hz]

    device = os.environ['COMEDI_DEVICE']
    inputSubdevice = os.environ['AI_SUBDEVICE']
    outputSubdevice = os.environ['AO_SUBDEVICE']
    inputChannels = []
    inputGains = []
    inputUnits = []
    outputChannels = []
    outputGains = []
    outputUnits = []
    reversalPotentials = []
    outputFilename = None
    resetOutput = None
    shuffle = False
    verbose = 4
    niceness = 0
    suffix = 'CC'

    offsets = []

    realtime = os.environ['LCG_REALTIME'].lower() == 'yes'
    terminalPrintMode = 'progress'

    model = None
    dry_run = False

    # parse arguments
    for o,a in opts:
        if o in ('-h','--help'):
            usage()
            sys.exit(0)
        elif o in ('-V','--voltage-clamp'):
            suffix = 'VC'
        elif o in ('-s','--stimulus'):
            stimfiles = []
            for f in a.split(','):
                stimfiles.append(f)
        elif o in ('-d','--directory'):
            stimdir = a
        elif o in ('-l','--duration'):
            duration_option = o
            duration = float(a)
        elif o in ('-n','--repetitions'):
            repetitions = int(a)
            if repetitions <= 0:
                print('The number of repetitions must be at least one.')
                sys.exit(1)
        elif o in ('-i','--interval'):
            interval = float(a)
            if interval < 0:
                print('The interval between repetitions must be non-negative.')
                sys.exit(1)
        elif o in ('-F','--sampling-rate'):
            samplingRate = float(a)
            if samplingRate <= 0:
                print('The sampling rate must be positive.')
                sys.exit(1)
        elif o in ('-D','--device'):
            if not os.path.exists(a):
                print('Device \'%s\' does not exist.' % a)
                sys.exit(1)
            device = a
        elif o == '--input-subdevice':
            inputSubdevice = a
        elif o == '--output-subdevice':
            outputSubdevice = a
        elif o in ('-I','--input-channels'):
            if a.lower() == 'none':
                # no input channels
                inputChannels = None
            elif '-' in a:
                # a range of input channels
                inputChannels = range(int(a.split('-')[0]),int(a.split('-')[1])+1)
            elif ',' in a:
                # a list of input channels
                for chan in a.split(','):
                    inputChannels.append(int(chan))
            else:
                # just one input channel
                inputChannels.append(int(a))
        elif o in ('-g','--input-gains'):
            for gain in a.split(','):
                inputGains.append(float(gain))
        elif o in ('-u','--input-units'):
            for unit in a.split(','):
                inputUnits.append(unit)
        elif o in ('-O','--output-channels'):
            if a.lower() == 'none':
                # no output channels
                outputChannels = None
            elif '-' in a:
                # a range of output channels
                outputChannels = range(int(a.split('-')[0]),int(a.split('-')[1])+1)
            elif ',' in a:
                # a list of output channels
                for chan in a.split(','):
                    outputChannels.append(int(chan))
            else:
                # just one output channel
                outputChannels.append(int(a))
        elif o in ('-G','--output-gains'):
            for gain in a.split(','):
                outputGains.append(float(gain))
        elif o in ('-U','--output-units'):
            for unit in a.split(','):
                outputUnits.append(unit)
        elif o in ('-E','--conductance'):
            for E in a.split(','):
                reversalPotentials.append(E)
        elif o in ('-H','--offset'):
            for offset in a.split(','):
                offsets.append(float(offset))
        elif o in ('-o','--output-file'):
            outputFilename = a
        elif o == '--rt':
            realtime = a.lower() == 'yes'
        elif o in ('-R','--reset-output'):
            resetOutput = a.lower() == 'yes'
        elif o == '--shuffle':
            shuffle = True
        elif o == '--verbose':
            if a == 'progress':
                terminalPrintMode = 'progress'
            if a == 'timer':
                terminalPrintMode = 'timer'
            elif a == 'percent':
                terminalPrintMode = 'percent'
            elif a == 'quiet':
                terminalPrintMode = 'quiet'                
            else:
                verbose = int(a)
        elif o == '--model':
            model = 'LIF'                
            realtime = 'yes'
            print(' LIf simulation mode.')
        elif o == '--dry-run':
            dry_run = True
        elif o in  ('-p','--priority'):
            niceness = int(a)
    # set higher priority for this process 
    os.nice(niceness)

    if inputChannels is None and outputChannels is None:
        print('No input or output channels specified. I cowardly refuse to continue.')
        sys.exit(0)

    if not outputChannels is None and not duration is None and (not stimfiles is None or not stimdir is None):
        print('Warning: if at least one output channel is specified, the duration of the recording')
        print('is given by the duration of the stimulus. Ignoring your %s option.' % duration_option)

    if outputChannels is None and len(offsets) != 0:
        print('You specified an offset but no output channel(s). I don\'t know what to do.')
        sys.exit(0)

    if inputSubdevice == outputSubdevice:
        print('Input and output subdevices must be different.')
        sys.exit(0)

    # check the correctness of the arguments
    if not inputChannels is None:
        if len(inputChannels) == 0:
            inputChannels = [int(os.environ['AI_CHANNEL_' + suffix])]
        if len(inputGains) == 0:
            inputGains = [float(os.environ['AI_CONVERSION_FACTOR_' + suffix]) for i in range(len(inputChannels))]
        if len(inputUnits) == 0:
            inputUnits = [os.environ['AI_UNITS_' + suffix] for i in range(len(inputChannels))]
        if len(inputChannels) != len(inputGains) or len(inputChannels) != len(inputUnits):
            print('The number of input channels, input gains and input units must be the same.')
            sys.exit(1)
    if not outputChannels is None:
        if len(outputChannels) == 0:
            outputChannels = [int(os.environ['AO_CHANNEL_' + suffix])]
        if len(outputGains) == 0:
            outputGains = [float(os.environ['AO_CONVERSION_FACTOR_' + suffix]) for i in range(len(outputChannels))]
        if len(outputUnits) == 0:
            outputUnits = [os.environ['AO_UNITS_' + suffix] for i in range(len(outputChannels))]
        if len(offsets) == 0:
            offsets = [0. for i in range(len(outputChannels))]
        if len(outputChannels) != len(outputGains) or len(outputChannels) != len(outputUnits) or len(outputChannels) != len(offsets):
            print('The number of output channels, output gains, output units and offsets must be the same.')
            sys.exit(1)

        if stimfiles is None and stimdir is None:
            # no -s or -d option: check whether an offset and a duration were specified...
            if len(offsets) > 0 and not duration is None:
                stimfiles = ['/tmp/tmp.stim']
                sub.call('lcg stimgen -o %s dc -d %g 0' % (stimfiles[0],duration), shell=True)
            else:
                # or whether a stimulus file was piped from lcg-stimgen
                stimulus = sys.stdin.read()
                if len(stimulus):
                    stimfiles = ['/tmp/tmp.stim']
                    with open(stimfiles[0],'w') as fid:
                        fid.write(stimulus)
                else:
                    print('You must specify one of -s or -d.')
                    sys.exit(0)

        if stimdir is not None:
            if stimfiles is not None:
                print('You cannot specify both -s and -D at the same time.')
                sys.exit(1)
            if stimdir[-1] == '/':
                stimdir = stimdir[:-1]
            if not os.path.isdir(stimdir):
                print('%s: no such directory.' % stimdir)
                sys.exit(1)
            stimfiles = glob.glob(stimdir + '/*.stim')
            stimfiles.sort()
            if len(stimfiles) == 0:
                print('The directory %s contains no stimulus files.' % stimdir)
                sys.exit(0)
        else:
            for f in stimfiles:
                if not os.path.isfile(f):
                    print('%s: no such file.' % f)
                    sys.exit(1)

        # if there is one stimfile and many output channels, we output the same stimulus
        # to all of them
        if len(stimfiles) == 1 and len(outputChannels) > 1:
            # doesn't make sense to shuffle in this case
            shuffle = False
            stimfiles = [stimfiles[0] for i in range(len(outputChannels))]

        if len(outputChannels) != 1 and len(stimfiles) != len(outputChannels):
            print('There are %d output channels and %d stimulus files: I don\'t know what to do.' % (len(outputChannels),len(stimfiles)))
            sys.exit(1)

    if len(reversalPotentials)>0 and suffix == 'VC':
        print('Conductance clamp requires current clamp mode. Stopping here...')
        sys.exit(1)

    # default value of resetOutput, for voltage and current clamp stimulations
    if resetOutput is None:
        if suffix == 'CC':
            resetOutput = os.environ['LCG_RESET_OUTPUT'].lower() == 'yes'
        else:
            resetOutput = False

    if outputChannels is None or len(stimfiles) == len(outputChannels):
        total = repetitions
    else:
        total = repetitions * len(stimfiles)
    cnt = 1
    if len(reversalPotentials) == 0:
        writeConfigurationFile = lambda cfile,dur,chan:lcg.writeIOConfigurationFile(
            cfile,
            samplingRate,
            dur,
            chan,
            realtime,
            outputFilename,
            model)
    else:
        writeConfigurationFile = lambda cfile,dur,chan:lcg.writeConductanceStimulusConfigurationFile(
            cfile,
            samplingRate,
            dur,
            chan,
            reversalPotentials,
            model)
    all_channels = []
    all_durations = []
    if outputChannels is None or len(stimfiles) == len(outputChannels):
        tmpchannels = []
        # input channels
        if not inputChannels is None:
            tmpchannels = [{'type':'input', 'subdevice': inputSubdevice, 'channel':inputChannels[j],
                            'factor':inputGains[j], 'units':inputUnits[j]} for j in range(len(inputChannels))]
        if not outputChannels is None:
            # there are as many stimulus files as output channels
            duration = get_stimulus_duration(stimfiles[0])
            differentDurations = False
            for f in stimfiles:
                d = get_stimulus_duration(f)
                if d != duration:
                    differentDurations = True
                    if d > duration:
                        duration = d
            if i == 0 and differentDurations:
                print('Warning: not all stimulus files have the same duration. Will use the longest, %g sec.' % duration)
            for j in range(len(outputChannels)):
                tmpchannels.append({'type':'output', 'subdevice': outputSubdevice, 'channel':outputChannels[j], 'factor':outputGains[j],
                                    'units':outputUnits[j], 'stimfile':stimfiles[j], 'offset':offsets[j], 'resetOutput': resetOutput})
        all_channels.append(tmpchannels)
        all_durations.append(duration)
    else:
        # there is one output channel and many stimulus files
        for f in stimfiles:
            try:
                all_durations.append(np.sum(np.loadtxt(f)[:,0]))
            except:
                all_durations.append(np.loadtxt(f)[0])
            tmpchannels = [{'type':'input', 'subdevice': inputSubdevice, 'channel':inputChannels[j], 'factor':inputGains[j],
                            'units':inputUnits[j]} for j in range(len(inputChannels))]
            tmpchannels.append({'type':'output', 'subdevice': outputSubdevice, 'channel':outputChannels[0], 'factor':outputGains[0],
                                'units':outputUnits[0], 'stimfile':f, 'offset':offsets[0], 'resetOutput': resetOutput})
            all_channels.append(tmpchannels)

    # Prepare run commands
    cmd_str = ('nice -n '+str(niceness) + ' ' +
               lcg.common.prog_name + ' -V ' + 
               str(verbose)+' -c ' + config_file)
    timerString = '\rTrial {0:d} of {1:d}'
    if dry_run:
        cmdstr = 'echo "' + cmd_str + '"'
    if (verbose != 4) or (terminalPrintMode == 'quiet'):
        runLCG = lambda string,count,total,dur: runCommand(cmd_str,mode=None)
    elif terminalPrintMode == 'progress':
        runLCG = lambda string,count,total,dur: runCommand(cmd_str,'progress',duration, 
                                                           string +' ')
    elif terminalPrintMode == 'timer':
        runLCG = lambda string,count,total,dur: runCommand(cmd_str,'timer',duration, 
                                                           string + '; elapsed time: {0:.2f}s ')
    elif terminalPrintMode == 'trial_bar':
        runLCG = lambda string,count,total,dur: runCommand(cmd_str,'trial_bar', 
                                                           count,
                                                           total,
                                                           string + ' ')        
    sys.stdout.flush()
    # Main loop
    for i in range(repetitions):
        if shuffle:
            random.shuffle(stimfiles)
            for stimfile,channels in zip(stimfiles,all_channels):
                for channel in channels:
                    if 'stimfile' in channel:
                        channel['stimfile'] = stimfile
        for duration,channels in zip(all_durations,all_channels):
            trialStart = time.time()
            writeConfigurationFile(config_file, duration, channels)        
            runLCG(timerString.format(cnt, total), cnt, total, duration)
            sleepTime = (interval + duration - (time.time() - trialStart))
            if cnt < total and sleepTime > 0 :
                time.sleep(sleepTime)
            cnt += 1
    sys.stdout.write('\n')

if __name__ == '__main__':
    main()
