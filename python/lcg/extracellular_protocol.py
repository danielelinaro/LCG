#!/usr/bin/env python

import os
import sys
import getopt
import subprocess as sub
import numpy as np
import lcg

stim_file = 'pulses{0}.stim'
cfg_file = 'pulses.cfg'
def usage():
    print('\nUsage: %s [option <value>]' % os.path.basename(sys.argv[0]))
    print('\nwhere options are:\n')
    print('     -h    display this help message and exit.')
    print('     -f    frequency of pulses (multiple frequencies will be used as intra,inter burtst).')
    print('     -n    number of pulses (default 10).')
    print('     -b    number of bursts (default 1).')
    print('     -d    duration of each pulse (default 0.1 msec).')
    print('     -a    amplitude of each pulse (default 5 V).')
    print('     -i    interval between trials (default 5 sec).')
    print('     -N    number of repetitions (default 20).')
    print('     -F    sampling rate (default 20000).')
    print('     -I    input channel (default 0).')
    print('     -O    output channel for extracellular stimulation (default 1).')
    print('')
    print(' --without-recovery-pulse    do not include a recovery pulse in the stimulation.')
    print(' --compute-kernel            run a kernel protocol for the input.')
    print('\nIn case the --compute-kernel option is specified, the following option is accepted:\n')
    print('     -o    output channel for kernel computation (default 0).')
    print('')

def main():
    try:
        opts,args = getopt.getopt(sys.argv[1:], 'hf:n:d:a:b:i:N:I:O:o:H:',
                                  ['help','compute-kernel','without-recovery-pulse'])
    except getopt.GetoptError, err:
        print str(err)
        usage()
        sys.exit(1)

    repetitions = 20      # [1]
    interval = 5          # [s]
    ai = [0]
    ao = [1]
    stim_freq = None      # [Hz]
    stim_dur = 0.1        # [ms]
    stim_amp = 10         # [V]
    npulses = 10
    nburst = 1
    holding = 0
    srate = 20000
    with_recovery = False
    compute_kernel = False
    extracellular_conversion_factor = 1
    for o,a in opts:
        if o in ('-h','--help'):
            usage()
            sys.exit(0)
        elif o == '-f':
            stim_freq = [np.float(i) for i in a.split(',')]
        elif o == '-n':
            npulses = int(a)
        elif o == '-b':
            nburst = int(a)
        elif o == '-d':
            stim_dur = float(a)
        elif o == '-i':
            interval = float(a)
        elif o == '-a':
            stim_amp = float(a)
        elif o == '-N':
            repetitions = int(a)
        elif o == '-I':
            ai = [int(i) for i in a.split(',')]
        elif o == '-O':
            ao = [int(i) for i in a.split(',')]
        elif o == '-F':
            srate = int(a)
        elif o == '-H':
            holding = float(a)
        elif o == '--compute-kernel':
            compute_kernel = True
        elif o == '--without-recovery-pulse':
            with_recovery = False

    run = lambda p:sub.call(p,shell=True)
    #run = lambda p:sys.stdout.write(str(p)+'\n')

    if not stim_freq:
        print('You must specify the stimulation frequency (-f switch).')
        sys.exit(1)
    if len(ao) < 2 and not holding == 0.0:
        print "Wrong number of channels specified -O must have"
        print" intracellular,extracellular channels when used with the -H option."
        sys.exit(1)

    comma = lambda y:','.join([str(i) for i in y])    
    stimnames = ','.join([stim_file.format(i) for i in range(len(ao))])
    duration = lcg.writePulsesStimFile(stim_freq, stim_dur, stim_amp, npulses,delay = 1,
                                       pulses_in_burst=nburst, withRecovery=with_recovery,
                                       filename=stimnames.split(',')[-1])
    
    if len(ao) + len(ai) > 2:
        print("Using lcg-non-rt")
        if compute_kernel and len(ao)>1:
            run('lcg kernel -I {0} -O {1} --non-rt -F {2}'.format(ai[0],ao[0],srate))

        run('lcg-rcwrite -e -i -c {0} --non-rt --file {1}'.format(comma(ai),cfg_file))
        if len(ao)>1:
            run('lcg-rcwrite -o -f 1 -u V -c {0} --non-rt -f {1},{2} -u pA,ND --file {3} -p {4}'.format(comma(ao),
                                                                                                        os.environ['AO_CONVERSION_FACTOR'],
                                                                                                        extracellular_conversion_factor,
                                                                                                        cfg_file,stimnames))
        else:
            run('lcg-rcwrite -o -f 1 -u V -c {0} --non-rt -f {1} -u ND --file {3} -p {4}'.format(comma(ao),
                                                                                                 os.environ['AO_CONVERSION_FACTOR'],
                                                                                                 cfg_file,stimnames))
        noise = True
        if noise:
            holdstim = [[1,1,0,0,0,0,0,0,0,0,0,0,1],[duration-2,2,0,holding,10,0,0,0,0,0,0,0,1],[1,1,0,0,0,0,0,0,0,0,0,0,1]]
        else:
            holdstim = [[duration,1,holding,0,0,0,0,0,0,0,0,1]]
        lcg.writeStimFile(stimnames.split(',')[0], holdstim, False)
        run('lcg-non-rt -c {0} -F {1} -n {2} -i {3}'.format(cfg_file,
                                                            srate,
                                                            repetitions,
                                                           interval))
    else:        
        run('lcg-rcwrite -e -i -c {0}'.format(comma(ai)))
        run('lcg-rcwrite -o -f 1 -u V -c {0} -f {1} -u ND --file {3} -p {4}'.format(comma(ao),
                                                                                    os.environ['AO_CONVERSION_FACTOR'],
                                                                                    cfg_file,stimnames))
        run('lcg vcclamp -f ' + stimnames + ' -n ' + str(repetitions) + ' -i ' + str(interval))

if __name__ == '__main__':
    main()
