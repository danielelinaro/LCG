#!/usr/bin/env python

import os
import sys
import getopt
import numpy as np
import lcg

extracellular_stim_file = 'extra.stim'
intracellular_stim_file = 'intra.stim'
outfile = 'ltp_ltd.xml'

def usage():
    print('\nUsage: %s [option <value>]' % os.path.basename(sys.argv[0]))
    print('\nwhere options are:\n')
    print('     -h     display this help message and exit.')
    print('     -T     period of the stimulation.')
    print('     -a     amplitude of intracellular pulse.')
    print('     -n     number of repetitions.')
    print('     -d     duration of each pulse (default 0.1 msec).')
    print('')
    print(' --pairing  pairing protocol.')
    print('     -D     time difference between extracellular and intracellular pulse (in msec).')
    print('')

def main():
    try:
        opts,args = getopt.getopt(sys.argv[1:], 'hT:n:d:a:D:',
                                  ['help','pairing'])
    except getopt.GetoptError, err:
        print str(err)
        usage()
        sys.exit(1)

    repetitions = None    # [1]
    pre = 0.5             # [s]
    post = 1              # [s]
    interval = None       # [s]
    ai = 0
    intra_ao = 0
    extra_ao = 1
    stim_freq = None      # [Hz]
    extra_stim_dur = 0.1  # [ms]
    intra_stim_dur = 1    # [ms]
    extra_stim_amp = 5    # [V]
    intra_stim_amp = None # [pA]
    pairing = False
    offset = None         # [pA]

    for o,a in opts:
        if o in ('-h','--help'):
            usage()
            sys.exit(0)
        elif o == '-T':
            interval = float(a)
        elif o == '-n':
            repetitions = int(a)
        elif o == '-d':
            extra_stim_dur = float(a)
        elif o == '-D':
            offset = float(a)
        elif o == '-a':
            intra_stim_amp = float(a)
        elif o == '--pairing':
            pairing = True

    if pairing and not offset:
        print('You must specify the time difference between extracellular and intracellular pulses (-D switch).')
        sys.exit(1)

    if pairing and not intra_stim_amp:
        print('You must specify the amplitude of the intracellular pulse (-a switch).')
        sys.exit(1)

    if not interval:
        print('You must specify the period of the stimulation (-T switch).')
        sys.exit(1)

    if not repetitions:
        print('You must specify the number of repetitions (-n switch).')
        sys.exit(1)

    extra_stim_dur = extra_stim_dur * 1e-3    # [s]

    extra_stim = [[pre,1,0,0,0,0,0,0,5061983,0,0,1],
                  [extra_stim_dur,1,extra_stim_amp,0,0,0,0,0,5061983,0,0,1],
                  [post-extra_stim_dur,1,0,0,0,0,0,0,5061983,0,0,1]]
    lcg.writeStimFile(extracellular_stim_file, extra_stim, False)

    if pairing:
        intra_stim_dur = intra_stim_dur * 1e-3    # [s]
        offset = offset * 1e-3    # [s]
        intra_stim = [[pre+offset,1,0,0,0,0,0,0,5061983,0,0,1],
                      [intra_stim_dur,1,intra_stim_amp,0,0,0,0,0,5061983,0,0,1],
                      [post-offset-intra_stim_dur,1,0,0,0,0,0,0,5061983,0,0,1]]
        lcg.writeStimFile(intracellular_stim_file, intra_stim, False)
        os.system('kernel_protocol -I ' + str(ai) + ' -O ' + str(intra_ao))
        try:
            infile = os.environ['CONFIGURATIONS_PATH'] + '/' + outfile
        except:
            infile = os.environ['HOME'] + '/configurations/' + outfile
        lcg.substituteStrings(infile, outfile, {'TEND': pre+post})
        os.system('dclamp -c '+ outfile + ' -n ' + str(repetitions) + ' -i ' + str(interval-(pre+post)))
    else:
        os.system('cclamprc_write -e -i -c ' + str(ai))
        os.system('cclamprc_write -o -f 1 -u V -c ' + str(extra_ao))
        os.system('cclamp -f ' + extracellular_stim_file + ' -n ' + str(repetitions) + ' -i ' + str(interval-(pre+post)))

if __name__ == '__main__':
    main()
