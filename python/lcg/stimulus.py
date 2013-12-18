#!/usr/bin/env python

import getopt
import glob
import lcg
import numpy as np
import os
import subprocess as sub
import sys

configFile = 'stimulus.xml'

def usage():
    print('')
    print('This program applies the stimulation described by one or more stimulus files')
    print('to the specified output channel(s), while at the same time recording from the')
    print('corresponding input channels, located on the same subdevice.')
    print('')
    print('Usage: %s [option <value>]' % os.path.basename(sys.argv[0]))
    print('')
    print('where options are:')
    print('')
    print(' -h, --help            display this help message and exit.')
    print(' -s, --stimulus        stimulus file(s) to use.')
    print(' -d, --directory       directory where the stimulus files are located.')
    print(' -n, --repetitions     number of repetitions (default 1).')
    print(' -i, --interval        interval between repetitions (default 0 s).')
    print(' -F, --sampling-rate   sampling frequency (default %s Hz)' % os.environ['SAMPLING_RATE'])
    print(' -D, --device          input device (default %s).' % os.environ['COMEDI_DEVICE'])
    print(' -S, --subdevice       input subdevice (default %s).' % os.environ['AI_SUBDEVICE'])
    print(' -I, --input-channels  input channels (comma separated values, default %s)' % os.environ['AI_CHANNEL'])
    print(' -g, --input-gains     input conversion factors (comma separated values, default %s' % os.environ['AI_CONVERSION_FACTOR_CC'])
    print('                       (or %s if --vclamp is used) for all channels).' % os.environ['AI_CONVERSION_FACTOR_VC'])
    print(' -u, --input-units     input units (comma separated values, default %s (or %s if' % (os.environ['AI_UNITS_CC'],os.environ['AI_UNITS_VC']))
    print('                       --vclamp is used) for all channels).')
    print(' -O, --output-channels output channels (comma separated values, default %s)' % os.environ['AO_CHANNEL'])
    print(' -G, --output-gains    output conversion factors (comma separated values, default %s' % os.environ['AO_CONVERSION_FACTOR_CC'])
    print('                       (or %s if --vclamp is used) for all channels).' % os.environ['AO_CONVERSION_FACTOR_VC'])
    print(' -U, --output-units    output units (comma separated values, default %s (or %s if' % (os.environ['AO_UNITS_CC'],os.environ['AO_UNITS_VC']))
    print('                       --vclamp is used) for all channels).')
    print(' -V, --vclamp          use default conversion factor and units for voltage clamp.')
    print('')

def main():
    try:
        opts,args = getopt.getopt(sys.argv[1:], 'hs:d:n:i:F:D:S:I:g:u:O:G:U:V',
                                  ['help','stimulus=','directory=','repetitions=','interval=','sampling-rate=',
                                   'device=','subdevice=',
                                   'input-channels=','input-gains=','input-units=',
                                   'output-channels=','output-gains=','output-units=',
                                   'vclamp'])
    except getopt.GetoptError, err:
        print(str(err))
        usage()
        sys.exit(1)

    stimfiles = None
    stimdir = None
    repetitions = 1
    interval = 0       # [s]

    samplingRate = float(os.environ['SAMPLING_RATE'])    # [Hz]

    device = os.environ['COMEDI_DEVICE']
    subdevice = os.environ['AI_SUBDEVICE']
    inputChannels = []
    inputGains = []
    inputUnits = []
    outputChannels = []
    outputGains = []
    outputUnits = []

    suffix = 'CC'

    for o,a in opts:
        if o in ('-h','--help'):
            usage()
            sys.exit(0)
        elif o in ('-s','--stimulus'):
            stimfiles = []
            for f in a.split(','):
                stimfiles.append(f)
        elif o in ('-d','--directory'):
            stimdir = a
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
            if interval <= 0:
                print('The sampling rate must be positive.')
                sys.exit(1)
        elif o in ('-D','--device'):
            if not os.path.exists(a):
                print('Device \'%s\' does not exist.' % a)
                sys.exit(1)
            device = a
        elif o in ('-S','--subdevice'):
            subdevice = a
        elif o in ('-I','--input-channels'):
            for chan in a.split(','):
                inputChannels.append(int(chan))
        elif o in ('-g','--input-gains'):
            for gain in a.split(','):
                inputGains.append(float(gain))
        elif o in ('-u','--input-units'):
            for unit in a.split(','):
                inputUnits.append(unit)
        elif o in ('-O','--output-channels'):
            for chan in a.split(','):
                outputChannels.append(int(chan))
        elif o in ('-G','--output-gains'):
            for gain in a.split(','):
                outputGains.append(float(gain))
        elif o in ('-U','--output-units'):
            for unit in a.split(','):
                outputUnits.append(unit)
        elif o in ('-V','--vclamp'):
            suffix = 'VC'

    if len(inputChannels) == 0:
        inputChannels = [int(os.environ['AI_CHANNEL'])]

    if len(inputGains) == 0:
        inputGains = [float(os.environ['AI_CONVERSION_FACTOR_' + suffix]) for i in range(len(inputChannels))]

    if len(inputUnits) == 0:
        inputUnits = [os.environ['AI_UNITS_' + suffix] for i in range(len(inputChannels))]

    if (len(inputChannels) != len(inputGains)) or (len(inputChannels) != len(inputUnits)):
        print('The number of input channels, input gains and input units must be the same.')
        sys.exit(1)

    if len(outputChannels) == 0:
        outputChannels = [int(os.environ['AO_CHANNEL'])]

    if len(outputGains) == 0:
        outputGains = [float(os.environ['AO_CONVERSION_FACTOR_' + suffix]) for i in range(len(outputChannels))]

    if len(outputUnits) == 0:
        outputUnits = [os.environ['AO_UNITS_' + suffix] for i in range(len(outputChannels))]
    
    if (len(outputChannels) != len(outputGains)) or (len(outputChannels) != len(outputUnits)):
        print('The number of output channels, output gains and output units must be the same.')
        sys.exit(1)

    if stimfiles is None and stimdir is None:
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
        if len(stimfiles) == 0:
            print('The directory %s contains no stimulus files.' % stimdir)
            sys.exit(0)
    else:
        for f in stimfiles:
            if not os.path.isfile(f):
                print('%s: no such file.' % f)
                sys.exit(1)

    if len(outputChannels) != 1 and len(stimfiles) != len(outputChannels):
        print('There are %d output channels and %d stimulus files: I don\'t know what to do.' % (len(outputChannels),len(stimfiles)))
        sys.exit(1)

    if len(stimfiles) == len(outputChannels):
        total = repetitions
    else:
        total = repetitions * len(stimfiles)
    cnt = 1
    for i in range(repetitions):
        if len(stimfiles) == len(outputChannels):
            # there are as many stimulus files as output channels
            duration = np.sum(np.loadtxt(stimfiles[0])[:,0])
            differentDurations = False
            for f in stimfiles:
                d = np.sum(np.loadtxt(f)[:,0])
                if d != duration:
                    differentDurations = True
                    if d > duration:
                        duration = d
            if not i and differentDurations:
                print('Warning: not all stimulus files have the same duration. Will use the longest, %g sec.' % duration)
            channels = [{'type':'input', 'channel':inputChannels[j], 'factor':inputGains[j],
                         'units':inputUnits[j]} for j in range(len(inputChannels))]
            for j in range(len(outputChannels)):
                channels.append({'type':'output', 'channel':outputChannels[j], 'factor':outputGains[j],
                                 'units':outputUnits[j], 'stimfile':stimfiles[j]})
            lcg.writeIOConfigurationFile(configFile,samplingRate,duration,channels)
            sys.stdout.write('\rTrial %02d/%02d ' % (cnt,total))
            sys.stdout.flush()
            sub.call(lcg.common.prog_name + ' -c ' + configFile, shell=True)
            if cnt < total:
                sub.call('sleep ' + str(interval), shell=True)
            else:
                sys.stdout.write('\n')
            cnt += 1
        else:
            # there is one output channel and many stimulus files
            for f in stimfiles:
                duration = np.sum(np.loadtxt(f)[:,0])
                channels = [{'type':'input', 'channel':inputChannels[j], 'factor':inputGains[j],
                             'units':inputUnits[j]} for j in range(len(inputChannels))]
                channels.append({'type':'output', 'channel':outputChannels[0], 'factor':outputGains[0],
                                 'units':outputUnits[0], 'stimfile':f})
                lcg.writeIOConfigurationFile(configFile,samplingRate,duration,channels)
                sys.stdout.write('\rTrial %02d/%02d ' % (cnt,total))
                sys.stdout.flush()
                sub.call(lcg.common.prog_name + ' -c ' + configFile, shell=True)
                if cnt < total:
                    sub.call('sleep ' + str(interval), shell=True)
                else:
                    sys.stdout.write('\n')
                cnt += 1

if __name__ == '__main__':
    main()
