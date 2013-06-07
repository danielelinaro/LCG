#!/usr/bin/env python

import os
import sys
import getopt
import subprocess as sub
import numpy as np
import lcg

extracellular_stim_file = 'extra.stim'
intracellular_stim_file = 'intra.stim'
config_file = 'ltp_ltd.xml'

def usage():
    print('\nUsage: %s [option <value>]' % os.path.basename(sys.argv[0]))
    print('\nwhere options are:\n')
    print('     -h    display this help message and exit.')
    print('     -T    period of the stimulation (in s).')
    print('     -n    number of repetitions.')
    print('     -k    period, in minutes, at which a new kernel should be computed (default 10).')
    print('     -s    duration of the extracellular stimulation (default 0.1 ms).')
    print('     -I    input channel (default 0).')
    print('     -O    output channels, in the form 0,1 where 0 and 1 are')
    print('           the channels corresponding to the extracellular')
    print('           and intracellular stimulation, respectively (default 0,1).')
    print('\nIf the --pairing option is provided, a pairing protocol is performed and the')
    print('following additional options are accepted:\n')
    print('     -D    time difference between extracellular and intracellular pulse (in ms).')
    print('     -a    amplitude of the intracellular pulse (in pA).')
    print('     -d    duration of the intracellular pulse (defaul 1 ms).')
    print('')

def run_batch(repetitions, interval, stim_dur, stim_amp, hyperpolarizing_pulse, pre, post,
              ai, ao, pairing=False, offset=None):
    try:
        template_config_file = os.environ['CONFIGURATIONS_PATH'] + '/' + config_file
    except:
        template_config_file = os.environ['HOME'] + '/configurations/' + config_file
    lcg.substituteStrings(template_config_file, config_file, {'TEND': pre+post,
                                                              'AI': ai,
                                                              'AO_INTRA': ao['intra'],
                                                              'AO_EXTRA': ao['extra']})
    extra_stim = [[pre,1,0,0,0,0,0,0,0,0,0,1],
                  [stim_dur['extra'],1,stim_amp['extra'],0,0,0,0,0,0,0,0,1],
                  [post-stim_dur['extra'],1,0,0,0,0,0,0,0,0,0,1]]
    lcg.writeStimFile(extracellular_stim_file, extra_stim, False)

    if pairing:
        intra_stim = [[pre+offset,1,0,0,0,0,0,0,0,0,0,1],
                      [stim_dur['intra'],1,stim_amp['intra'],0,0,0,0,0,0,0,0,1],
                      [post-offset-stim_dur['intra']-hyperpolarizing_pulse['dur']-0.1,1,0,0,0,0,0,0,0,0,0,1],
                      [hyperpolarizing_pulse['dur'],1,hyperpolarizing_pulse['amp'],0,0,0,0,0,0,0,0,1],
                      [0.1,1,0,0,0,0,0,0,0,0,0,1]]
    else:
        intra_stim = [[pre+post-hyperpolarizing_pulse['dur']-0.1,1,0,0,0,0,0,0,0,0,0,1],
                      [hyperpolarizing_pulse['dur'],1,hyperpolarizing_pulse['amp'],0,0,0,0,0,0,0,0,1],
                      [0.1,1,0,0,0,0,0,0,0,0,0,1]]

    lcg.writeStimFile(intracellular_stim_file, intra_stim, False)

    sub.call('lcg kernel -I ' + str(ai) + ' -O ' + str(ao['intra']), shell=True)
    sub.call(lcg.common.prog_name + ' -c '+ config_file + ' -n ' + str(repetitions) +
             ' -i ' + str(interval-(pre+post)), shell=True)

def main():
    try:
        opts,args = getopt.getopt(sys.argv[1:], 'hT:k:n:s:D:a:d:I:O:',
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
    ao = {'intra': 0, 'extra': 1}
    stim_freq = None      # [Hz]
    stim_dur = {'extra': 0.1, 'intra': 1}  # [ms]
    stim_amp = {'extra': 5,    # [V]
                'intra': None} # [pA]
    pairing = False
    offset = None         # [pA]
    kernel_period = 10    # [minutes]

    hyperpolarizing_pulse = {'dur': 0.3,  # [s]
                             'amp': -100} # [pA]

    for o,a in opts:
        if o in ('-h','--help'):
            usage()
            sys.exit(0)
        elif o == '-T':
            interval = float(a)
        elif o == '-k':
            kernel_period = float(a)
        elif o == '-n':
            repetitions = []
            for rep in a.split(','):
                repetitions.append(int(rep))
        elif o == '-s':
            stim_dur['extra'] = float(a)
        elif o == '-D':
            offset = float(a)
        elif o == '-a':
            stim_amp['intra'] = float(a)
        elif o == '-d':
            stim_dur['intra'] = float(a)
        elif o == '--pairing':
            pairing = True
        elif o == '-I':
            ai = int(a)
        elif o == '-O':
            channels = a.split(',')
            ao['intra'] = channels[0]
            ao['extra'] = channels[1]

    kernel_period *= 60   # convert to seconds

    if not repetitions:
        print('You must specify the number of repetitions (-n switch).')
        sys.exit(1)

    if len(repetitions) > 3:
        print('Don''t know what to do with %d batches.' % len(repetitions))
        sys.exit(1)

    if len(repetitions) > 1:
        pairing = True

    if pairing:
        if not offset:
            print('You must specify the time difference between extracellular and intracellular pulses (-D switch).')
            sys.exit(1)
        if not stim_amp['intra']:
            print('You must specify the amplitude of the intracellular pulse (-a switch).')
            sys.exit(1)

    if not interval:
        print('You must specify the period of the stimulation (-T switch).')
        sys.exit(1)

    stim_dur['extra'] = stim_dur['extra']*1e-3        # [s]
    if pairing:
        stim_dur['intra'] = stim_dur['intra']*1e-3    # [s]
        offset = offset * 1e-3                        # [s]

    if len(repetitions) == 1:
        run_batch(repetitions[0], interval, stim_dur, stim_amp, hyperpolarizing_pulse, pre, post, ai, ao, pairing, offset)
    else:
        os.mkdir('01',0755)
        os.chdir('01')
        while repetitions[0] > 0:
            reps = min(repetitions[0],int(round(kernel_period/interval)))
            run_batch(reps, interval, stim_dur, stim_amp, hyperpolarizing_pulse, pre, post, ai, ao, False)
            repetitions[0] -= reps
        os.chdir('..')
        os.mkdir('02',0755)
        os.chdir('02')
        while repetitions[1] > 0:
            reps = min(repetitions[1],int(round(kernel_period/interval)))
            run_batch(reps, interval, stim_dur, stim_amp, hyperpolarizing_pulse, pre, post, ai, ao, True, offset)
            repetitions[1] -= reps
        os.chdir('..')
        if len(repetitions) == 3:
            os.mkdir('03',0755)
            os.chdir('03')
            while repetitions[2] > 0:
                reps = min(repetitions[2],int(round(kernel_period/interval)))
                run_batch(reps, interval, stim_dur, stim_amp, hyperpolarizing_pulse, pre, post, ai, ao, False)
                repetitions[2] -= reps
            os.chdir('..')
        
if __name__ == '__main__':
    main()
