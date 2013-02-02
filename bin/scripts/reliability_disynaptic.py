#!/usr/bin/env python

import os
import sys
import getopt
import numpy as np
import dlutils as dl

duration_preamble = 2.61

def usage():
    try:
        templateFile = os.environ['CONFIGURATIONS_PATH'] + '/reliability_disynaptic_current.xml'
    except:
        templateFile = os.environ['HOME'] + '/configurations/reliability_disynaptic_current.xml'
    print('\nUsage: %s [option <value>]' % os.path.basename(sys.argv[0]))
    print('\nwhere options are:\n')
    print('   -h   display this help message and exit')
    print('   -m   the mean of the stimulation current in pA')
    print('   -s   the standard deviation of the stimulation current in pA')
    print('   -t   the autocorrelation time constant of the stimulation current in ms')
    print('   -c   the configuration file (default %s)' % templateFile)
    print('   -E   the reversal potential of the inhibitory synapse (default -80 mV)')
    print('   -w   the weight of the inhibitory synaptic connection')
    print('   -r   rise time of the inhibitory bi-exponential synapse (default 1 ms)')
    print('   -d   decay time of the inhibitory bi-exponential synapse (default 8 ms)')
    print('   -T   the duration of the stimulus (default 5 s)')
    print('   -n   the number of trials (default 10)')
    print('   -i   the interval between trials (default 30 s)')
    print('   -I   input channel (default 0).')
    print('   -O   output channel (default 0).\n')
    print('')

def writeStimFile(filename, stimulus):
    with open(filename,'w') as fid:
        for row in stimulus:
            for value in row:
                fid.write(str(value)+'\t')
            fid.write('\n')

def main():
    try:
        opts,args = getopt.getopt(sys.argv[1:],'hc:E:w:r:d:T:n:i:m:s:t:I:O', ['help'])
    except getopt.GetoptError, err:
        print str(err)
        usage()
        sys.exit(1)

    I_mean = None
    I_std = None
    I_tau = None
    weight = None
    config_file = None
    E = -80         # [mV]
    tau_r = 1e-3    # [s]
    tau_d = 8e-3    # [s]
    duration = 5    # [s]
    trials = 10     # [1]
    interval = 30   # [s]
    seed = 5061983  # [1]
    ai = 0
    ao = 0
    for o,a in opts:
        if o in ('-h','--help'):
            usage()
            sys.exit(0)
        elif o == '-m':
            I_mean = float(a)
        elif o == '-s':
            I_std = float(a)
        elif o == '-t':
            I_tau = float(a)
        elif o == '-c':
            config_file = a
        elif o == '-E':
            E = float(a)
        elif o == '-w':
            weight = float(a)
        elif o == '-r':
            tau_r = float(a)*1e-3
        elif o == '-d':
            tau_d = float(a)*1e-3
        elif o == '-T':
            duration = float(a)
        elif o == '-n':
            trials = int(a)
        elif o == '-i':
            interval = float(a)
        elif o == '-I':
            ai = int(a)
        elif o == '-O':
            ao = int(a)

    if I_mean == None or I_std == None or I_tau == None:
        print('You must specify the mean, the standard deviation and the\n' +
              'autocorrelation time constant of the stimulus current.')
        usage()
        sys.exit(1)

    if weight == None:
        print('You must specify the weight of the inhibitory connection.')
        usage()
        sys.exit(1)

    stimulus = [[0.5,1,0,0,0,0,0,0,0,0,0,1],
               [0.01,1,-300,0,0,0,0,0,0,0,0,1],
               [0.5,1,0,0,0,0,0,0,0,0,0,1],
               [0.6,1,-100,0,0,0,0,0,0,0,0,1],
               [1,1,0,0,0,0,0,0,0,0,0,1],
               [duration,2,I_mean,I_std,I_tau,0,0,1,seed,0,0,1],
               [1,1,0,0,0,0,0,0,0,0,0,1]]

    if config_file == None:
        try:
            template_file = os.environ['CONFIGURATIONS_PATH'] + '/reliability_disynaptic_current.xml'
        except:
            template_file = os.environ['HOME'] + '/configurations/reliability_disynaptic_current.xml'
        if os.path.isfile(template_file):
            config_file = 'reliability_disynaptic.xml'
            dl.substituteStrings(template_file, config_file,
                                 {'<weight>0</weight>': '<weight>'+str(weight)+'</weight>',
                                  '<E>-80</E>': '<E>'+str(E)+'</E>',
                                  '<tauRise>0</tauRise>': '<tauRise>'+str(tau_r)+'</tauRise>',
                                  '<tauDecay>0</tauDecay>': '<tauDecay>'+str(tau_d)+'</tauDecay>',
                                  '<tend>0</tend>': '<tend>'+str(np.sum(stimulus,0)[0])+'</tend>'})
        else:
            print('Default configuration file [%s] missing.' % template_file)
            usage()
            sys.exit(2)

    writeStimFile('current.stim',stimulus)
    os.system('kernel_protocol -I ' + str(ai) + ' -O ' + str(ao))
    os.system('dclamp -V 3 -c ' + config_file + ' -n ' + str(trials) + ' -i ' + str(interval))

if __name__ == '__main__':
    main()
