#!/usr/bin/env python

import os
import sys
import getopt
import subprocess as sub
import lcg

def usage():
    print('\nUsage: %s [option <value>]' % os.path.basename(sys.argv[0]))
    print('\nwhere options are:\n')
    print('          -h    display this help message and exit.')
    print('          -f    frequency of the stimulation.')
    print('          -O    output channel.')
    print('          -I    input channels (in the form 0,1 where 0 and 1 are')
    print('                the channels corresponding to the pre-synaptic and')
    print('                post-synaptic neurons, respectively).')
    print('          -n    number of pulses (default 10).')
    print('          -F    sampling frequency (default 15000).')
    print('          -d    stimulation duration (default 1 ms).')
    print('          -a    stimulation amplitude (default 4000 pA).')
    print('          -N    number of repetitions (default 10).')
    print('          -i    interval between repetitions (default 10 s).')
    print(' --no-kernel    do not compute the kernel.')
    print('    --non-rt    use the non-real-time version of the software.')
    print('')

def main():
    try:
        opts,args = getopt.getopt(sys.argv[1:], 'hN:n:f:F:d:a:I:O:', ['help','non-rt','no-kernel'])
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
    nonrt = False
    kernel = True

    for o,a in opts:
        if o in ('-h','--help'):
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
        elif o == '--non-rt':
            nonrt=True
        elif o == '--no-kernel':
            kernel=False

    
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

    if not nonrt:
        if kernel:
            sub.call('lcg kernel -a -F ' + str(fsampl) + ' -I ' + str(ai[0]) + ' -O ' + str(ao), shell=True)

        sub.call('cclamprc_write -e -o -c ' + str(ao), shell=True)
        sub.call('cclamprc_write -i -c ' + str(ai[0]), shell=True)
        sub.call('cclamprc_write -i -c ' + str(ai[1]), shell=True)

        lcg.writePulsesStimFile(fstim, stimdur, stimamp, npulses, delay=1, withRecovery=True, filename=stimfile)

        sub.call('lcg vcclamp -f ' + stimfile + ' -n ' + str(trials) + ' -i ' + str(interval), shell=True)
    else:
        if kernel:
            sub.call('lcg kernel -a -F ' + str(fsampl) + ' -I ' + str(ai[0]) 
                     + ' -O ' + str(ao) + ' --non-rt', shell=True)
        fname = 'pulses.cfg'
        sub.call('lcg-rcwrite -e -i -c ' + str(ai[0])+',' + str(ai[1]) + ' --non-rt -f '+ fname, shell=True)
        sub.call('lcg-rcwrite -o -c ' + str(ao)+' -p ' + stimfile + ' --non-rt -f '+ fname, shell=True)
        lcg.writePulsesStimFile(fstim, stimdur, stimamp, npulses, delay=1, withRecovery=True, filename=stimfile)

        sub.call('lcg-non-rt -c ' + fname + ' -n ' + str(trials) + ' -i ' + str(interval) + ' -F ' + str(fsampl), shell=True)


if __name__ == '__main__':
    main()
