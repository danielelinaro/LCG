#!/usr/bin/env python

import os
import sys
import getopt
import numpy as np
import dlutils as dl

def usage():
    try:
        templateFileWithoutBg = os.environ['CONFIGURATIONS_PATH'] + '/disynaptic_microcircuit.xml'
        templateFileWithBg = os.environ['CONFIGURATIONS_PATH'] + '/disynaptic_microcircuit_plus_bg.xml'
    except:
        templateFileWithoutBg = os.environ['HOME'] + '/disynaptic_microcircuit.xml'
        templateFileWithBg = os.environ['HOME'] + '/disynaptic_microcircuit_plus_bg.xml'
    print('\nUsage: %s [option <value>]' % sys.argv[0])
    print('\nwhere options are:\n')
    print('   -h   display this help message and exit')
    print('   -f   the frequency of the stimulation pulses.')
    print('   -a   the amplitude of the stimulation pulses (in pA).')
    print('   -w   the weight of the synaptic connection between the Martinotti cell and the post-synaptic pyramidal neuron.')
    print('   -c   the configuration file (default %s (w/o bg) or %s (with bg))' % (templateFileWithoutBg,templateFileWithBg))
    print('   -n   the number of trials (default 10)')
    print('   -i   the interval between trials (default 20 s)')
    print('   -I   input channels (comma separated values of pre and post channels, in that order).')
    print('   -O   output channels (comma separated values of pre and post channels, in that order).')
    print('   -B   this switch enables the addition of synaptic background noise (default is no).\n')
    print('The following switches are considered only if -B is present:')
    print('   -R   input resistance of the cell (in MOhm).')
    print('   -F   firing frequency of the excitatory background population.')
    print('   -v   value of voltage at which the background activity should be balanced.')

if __name__ == '__main__':

    try:
        opts,args = getopt.getopt(sys.argv[1:],'hBf:a:c:n:w:i:I:O:R:F:v:', ['help', 'output='])
    except getopt.GetoptError, err:
        print str(err)
        usage()
        sys.exit(1)

    stim_freq = None
    stim_amp = None
    config_file = None
    weight = None
    trials = 10     # [1]
    interval = 20   # [s]
    seed = 5061983  # [1]
    ai = []
    ao = []
    with_bg = False
    input_resistance = None # [MOhm]
    bg_freq = None          # [Hz]
    balanced_voltage = None # [mV]

    for o,a in opts:
        if o == '-h':
            usage()
            sys.exit(0)
        elif o == '-B':
            with_bg = True
        elif o == '-f':
            stim_freq = float(a)
        elif o == '-a':
            stim_amp = float(a)
        elif o == '-w':
            weight = float(a)
        elif o == '-c':
            config_file = a
        elif o == '-n':
            trials = int(a)
        elif o == '-i':
            interval = float(a)
        elif o == '-I':
            for ch in a.split(','):
                ai.append(int(ch))
        elif o == '-O':
            for ch in a.split(','):
                ao.append(int(ch))
        elif o == '-R':
            input_resistance = float(a)
        elif o == '-F':
            bg_freq = float(a)
        elif o == '-v':
            balanced_voltage = float(a)

    if not weight:
        print('You must specify the weight of the connection MC -> PYR (switch -w).')
        usage()
        sys.exit(1)

    if not stim_freq:
        print('You must specify the stimulation frequency (switch -f).')
        usage()
        sys.exit(1)

    if not stim_amp:
        print('You must specify the stimulation amplitude (switch -a).')
        usage()
        sys.exit(1)

    if not config_file:
        if with_bg:
            config_file = 'disynaptic_microcircuit_plus_bg.xml'
        else:
            config_file = 'disynaptic_microcircuit.xml'
        try:
            template_file = os.environ['CONFIGURATIONS_PATH'] + '/' + config_file
        except:
            template_file = os.environ['HOME'] + '/' + config_file

    if len(ai) != 2 or len(ao) != 2:
        print('You must specify the input and output channels in the form 0,1')
        print('where 0 is the channel of the pre-synaptic neuron and 1 of the')
        print('post-synaptic one.')
        usage()
        sys.exit(1)

    if with_bg and (not input_resistance or not bg_freq or not balanced_voltage):
        print('You need to specify the input resistance, the firing rate of the')
        print('excitatory background synaptic activity and the balanced voltage.')
        usage()
        sys.exit(1)

    stim_dur = dl.writePulsesStimFile(f=stim_freq, dur=1, amp=stim_amp, N=15, delay=1, withRecovery=False)
    
    if os.path.isfile(template_file):
        dl.substituteStrings(template_file, config_file,
                             {'WGT': str(weight),
                              'AI_PRE': str(ai[0]), 'AI_POST': str(ai[1]),
                              'AO_PRE': str(ao[0]), 'AO_POST': str(ao[1]),
                              'KERNEL_PRE': 'kernel-'+str(ai[0])+'-'+str(ao[0])+'.dat',
                              'KERNEL_POST': 'kernel-'+str(ai[1])+'-'+str(ao[1])+'.dat',
                              'TEND': str(stim_dur)})
    else:
        os.remove('pulses.stim')
        print('Default configuration file [%s] missing.' % template_file)
        usage()
        sys.exit(1)

    if with_bg:
        ratio = dl.computeRatesRatio(balanced_voltage, input_resistance)
        Gm_exc,Gm_inh,Gs_exc,Gs_inh = dl.computeSynapticBackgroundCoefficients(ratio, input_resistance, bg_freq)
        dl.writeGStimFiles({'m': Gm_exc, 's': Gs_exc, 'tau': 5, 'seed': 5061983},
                           {'m': Gm_inh, 's': Gs_inh, 'tau': 10, 'seed': 7051983},
                           stim_dur, 0, 0)

    os.system('kernel_protocol -a -F 15000 -I ' + str(ai[0]) + ' -O ' + str(ao[0]))
    os.system('kernel_protocol -a -F 15000 -I ' + str(ai[1]) + ' -O ' + str(ao[1]))
    os.system('dclamp -c ' + config_file + ' -n ' + str(trials) + ' -i ' + str(interval))

