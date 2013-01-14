#!/usr/bin/env python

import os
import sys
import dlutils as dl

def usage():
    print('\nUsage: %s [option <value>]' % os.path.basename(sys.argv[0]))
    print('\nwhere options are:\n')
    print('     -f    frequency of the stimulation')
    print('     -o    output channel')
    print('     -n    number of pulses (default 10)')
    print('     -F    sampling frequency (default 15000)')
    print('     -d    stimulation duration (default 1 ms)')
    print('     -a    stimulation amplitude (default 4000 pA)')
    print('     -N    number of repetitions (default 10)')
    print('     -i    interval between repetitions (default 10 s)')

def writeEntry(append, entry, subdevice, channel, conversion_factor, units):
    mode = 'w'
    if append:
        mode = 'a'
    with open(os.environ['HOME'] + '/.cclamprc',mode) as fid:
        fid.write('[' + entry + ']\n')
        fid.write('device = /dev/comedi0\n')
        fid.write('range = [-10,+10]\n')
        fid.write('subdevice = ' + str(subdevice) + '\n')
        fid.write('channel = ' + str(channel) + '\n')
        fid.write('conversionFactor = ' + str(conversion_factor) + '\n')
        fid.write('reference = GRSE\n')
        fid.write('units = ' + units + '\n\n')

def main():
    import getopt

    try:
        opts,args = getopt.getopt(sys.argv[1:], "hN:n:f:F:o:d:a:", ["help", "output="])
    except getopt.GetoptError, err:
        print str(err)
        usage()
        sys.exit(1)

    fstim = None
    output_channel = None

    trials = 10
    npulses = 10
    fsampl = 15000
    stimfile = 'pulses.stim'
    stimdur = 1
    stimamp = 4000
    interval = 10

    for o,a in opts:
        if o == '-h':
            usage()
            sys.exit(0)
        elif o == '-N':
            trials = int(a)
        elif o == '-n':
            npulses = int(a)
        elif o == '-f':
            fstim = float(a)
        elif o == '-F':
            fsampl = float(a)
        elif o == '-o':
            output_channel = int(a)
        elif o == '-d':
            stimdur = float(a)
        elif o == '-a':
            stimamp = float(a)
        elif o == '-i':
            interval = float(a)

    if fstim == None:
        print('You must specify the stimulation frequency')
        usage()
        sys.exit(1)

    if output_channel == None:
        print('You must specify the output channel')
        usage()
        sys.exit(1)

    writeEntry(False, 'AnalogInput0', 0, 0, 100, 'mV')
    writeEntry(True, 'AnalogInput1', 0, 1, 100, 'mV')
    writeEntry(True, 'AnalogOutput0', 1, output_channel, 0.001, 'pA')
    dl.writePulsesStimFile(fstim, stimdur, stimamp, npulses, delay=1, withRecovery=True, filename=stimfile)

    os.system('kernel_protocol -F ' + str(fsampl) + ' -i ' + str(output_channel) + ' -o ' + str(output_channel))
    os.system('cclamp -f ' + stimfile + ' -n ' + str(trials) + ' -i ' + str(interval))

if __name__ == '__main__':
    main()
