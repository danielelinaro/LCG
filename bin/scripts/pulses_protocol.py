#!/usr/bin/env python

import os
import sys
import dlutils as dl

def usage():
    print('\nUsage: %s [option <value>]' % os.path.basename(sys.argv[0]))
    print('\nwhere options are:\n')
    print('     -f    frequency of the stimulation')
    print('     -O    output channel')
    print('     -I    input channels (in the form 0,1 where 0 and 1 are')
    print('           the channels corresponding to the pre-synaptic and')
    print('           post-synaptic neurons, respectively.)')
    print('     -n    number of pulses (default 10)')
    print('     -F    sampling frequency (default 15000)')
    print('     -d    stimulation duration (default 1 ms)')
    print('     -a    stimulation amplitude (default 4000 pA)')
    print('     -N    number of repetitions (default 10)')
    print('     -i    interval between repetitions (default 10 s)')

if __name__ == '__main__':
    import getopt

    try:
        opts,args = getopt.getopt(sys.argv[1:], "hN:n:f:F:d:a:I:O:", ["help", "output="])
    except getopt.GetoptError, err:
        print str(err)
        usage()
        sys.exit(1)

    fstim = None
    ao = None
    ai = []

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
        elif o == '-d':
            stimdur = float(a)
        elif o == '-a':
            stimamp = float(a)
        elif o == '-i':
            interval = float(a)
        elif o == '-I':
            ai = []
            for ch in a.split(','):
                ai.append(int(ch))
        elif o == '-O':
            ao = int(a)

    
    if fstim == None:
        print('You must specify the stimulation frequency.')
        usage()
        sys.exit(1)

    if len(ai) != 2:
        print('You must specify the input channels.')
        usage()
        sys.exit(1)

    if ao == None:
        print('You must specify the output channel.')
        usage()
        sys.exit(1)

    os.system('kernel_protocol -a -F ' + str(fsampl) + ' -I ' + str(ai[0]) + ' -O ' + str(ao))

    os.system('cclamprc_write -e -o -c ' + str(ao))
    os.system('cclamprc_write -i -c ' + str(ai[0]))
    os.system('cclamprc_write -i -c ' + str(ai[1]))

    dl.writePulsesStimFile(fstim, stimdur, stimamp, npulses, delay=1, withRecovery=True, filename=stimfile)

    os.system('cclamp -f ' + stimfile + ' -n ' + str(trials) + ' -i ' + str(interval))
