#!/usr/bin/env python

import os
import sys
import getopt
import subprocess as sub
import lcg

configFile = 'spontaneous.xml'

def usage():
    print('')
    print('This program records spontaneous activity from an arbitrary number of')
    print('input channels, located on the same subdevice.')
    print('')
    print('Usage: %s [option <value>]' % os.path.basename(sys.argv[0]))
    print('')
    print('where options are:')
    print('')
    print(' -h, --help            display this help message and exit.')
    print(' -d, --duration        duration of the recording.')
    print(' -n, --repetitions     number of repetitions (default 1).')
    print(' -i, --interval        interval between repetitions (default 0 s).')
    print(' -F, --sampling-rate   sampling frequency (default %s Hz)' % os.environ['SAMPLING_RATE'])
    print(' -D, --device          input device (default %s).' % os.environ['COMEDI_DEVICE'])
    print(' -S, --subdevice       input subdevice (default %s).' % os.environ['AI_SUBDEVICE'])
    print(' -I, --input-channels  input channels (comma separated values, default %s)' % os.environ['AI_CHANNEL'])
    print(' -G, --input-gains     input conversion factors (comma separated values, default %s' % os.environ['AI_CONVERSION_FACTOR_CC'])
    print('                       (or %s if --vclamp is used) for all channels).' % os.environ['AI_CONVERSION_FACTOR_VC'])
    print(' -U, --input-units     input units (comma separated values, default %s (or %s if' % (os.environ['AI_UNITS_CC'],os.environ['AI_UNITS_VC']))
    print('                       --vclamp is used) for all channels).')
    print(' -V, --vclamp          use default conversion factor and units for voltage clamp.')
    print('')

def main():
    try:
        opts,args = getopt.getopt(sys.argv[1:], 'hd:n:i:F:D:S:I:G:U:V',
                                  ['help','duration=','repetitions=','interval=','sampling-rate=',
                                   'device=','subdevice=','input-channels=','input-gains=',
                                   'input-units=','vclamp'])
    except getopt.GetoptError, err:
        print(str(err))
        usage()
        sys.exit(1)

    duration = None    # [s]
    repetitions = 1
    interval = 1       # [s]

    samplingRate = float(os.environ['SAMPLING_RATE'])    # [Hz]

    device = os.environ['COMEDI_DEVICE']
    subdevice = os.environ['AI_SUBDEVICE']
    inputChannels = []
    inputGains = []
    inputUnits = []

    suffix = 'CC'

    for o,a in opts:
        if o in ('-h','--help'):
            usage()
            sys.exit(0)
        elif o in ('-d','--duration'):
            duration = float(a)
            if duration <= 0:
                print('The duration of the recording must be positive.')
                sys.exit(1)
        elif o in ('-n','--repetitions'):
            repetitions = int(a)
            if repetitions <= 0:
                print('The number of repetitions must be at least one.')
                sys.exit(1)
        elif o in ('-i','--interval'):
            interval = float(a)
            if interval <= 0:
                print('The interval between repetitions must be positive.')
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
        elif o in ('-G','--input-gains'):
            for gain in a.split(','):
                inputGains.append(float(gain))
        elif o in ('-U','--input-units'):
            for unit in a.split(','):
                inputUnits.append(unit)
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

    if duration is None:
        print('You must specify the duration of the recording (-d switch).')
        sys.exit(1)

    lcg.writeIOConfigurationFile(configFile,samplingRate,duration,
                                 [{'type':'input', 'channel':inputChannels[i], 'factor':inputGains[i], 'units':inputUnits[i]}
                                  for i in range(len(inputChannels))])

    for i in range(repetitions):
        sub.call(lcg.common.prog_name + ' -c ' + configFile, shell=True)
        if i < repetitions-1:
            sub.call('sleep ' + str(interval), shell=True)

if __name__ == '__main__':
    main()
