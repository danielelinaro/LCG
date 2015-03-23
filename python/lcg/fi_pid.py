#!/usr/bin/env python

import os
import sys
import getopt
import subprocess as sub
import lcg

config_file = 'fi_pid.xml'

def usage():
    print('\nUsage: %s [option <value>]' % os.path.basename(sys.argv[0]))
    print('\nwhere options are:\n')
    print('          -h    Display this help message and exit.')
    print('          -a    Initial amplitude of the injected current.')
    print('          -s    Standard deviation of the additionally injected noisy current (default 0 pA).')
    print('          -t    Time constant of the additionally injected noisy current (default 20 ms).')
    print('          -m    Minimal frequency (default 5 Hz).')
    print('          -M    Maximal frequency (default 30 Hz).')
    print('          -p    Proportional gain of the controller (default 0.01).')
    print('          -i    Integral gain of the controller (default 1).')
    print('          -d    Derivative gain of the controller (default 0).')
    print('          -T    Time constant of the frequency estimator (default 1 sec).')
    print('          -D    Duration of the protocol (default 30 sec).')
    print('          -n    Number of repetitions (default 1).')
    print('          -w    Interval between repetitions (default 60 sec).')
    print('          -F    Sampling rate (default %s Hz).' % os.environ['SAMPLING_RATE'])
    print('          -I    Input channel (default 0).')
    print('          -O    Output channel (default 0).')
    print(' --no-kernel    Do not compute the electrode kernel (assumes that a kernel.dat file is present).')
    print('')

def parseArgs():
    try:
        opts,args = getopt.getopt(sys.argv[1:], 'ha:m:M:s:t:p:i:d:T:D:n:w:F:I:O:', ['help','no-kernel','no-preamble'])
    except getopt.GetoptError, err:
        print str(err)
        usage()
        sys.exit(1)

    options = {'amplitude': None, 'current_tau': 20, 'max_freq': 30, 'min_freq': 5,
               'gp': 0.001, 'gi': 1, 'gd': 0, 'tau': 1, 'duration': 30,
               'trials': 1, 'interval': 60, 'sampling_rate': float(os.environ['SAMPLING_RATE']), 'ai': 0, 'ao': 0,
               'with_kernel': True}

    for o,a in opts:
        if o in ('-h','--help'):
            usage()
            sys.exit(0)
        elif o == '-a':
            options['amplitude'] = float(a)
        elif o == '-s':
            options['current_std'] = float(a)
        elif o == '-t':
            options['current_tau'] = float(a)
        elif o == '-m':
            options['min_freq'] = float(a)
        elif o == '-M':
            options['max_freq'] = float(a)
        elif o == '-p':
            options['gp'] = float(a)
        elif o == '-i':
            options['gi'] = float(a)
        elif o == '-d':
            options['gd'] = float(a)
        elif o == '-T':
            options['tau'] = float(a)
        elif o == '-D':
            options['duration'] = float(a)
        elif o == '-n':
            options['trials'] = int(a)
        elif o == '-w':
            options['interval'] = float(a)
        elif o == '-F':
            options['sampling_rate'] = float(a)
        elif o == '-I':
            options['ai'] = int(a)
        elif o == '-O':
            options['ao'] = int(a)
        elif o == '--no-kernel':
            options['with_kernel'] = False

    if options['amplitude'] is None:
        print('You must specify the initial amplitude of the injected current (-a switch).')
        sys.exit(1)
    
    return options

def writeFiles(options):
    # XML configuration file
    config = lcg.XMLConfigurationFile(options['sampling_rate'],options['duration'])
    config.add_entity(lcg.entities.H5Recorder(id=0, connections=(), compress=True))
    config.add_entity(lcg.entities.RealNeuron(id=1, connections=(0,4), spikeThreshold=-20, V0=-65, deviceFile=os.environ['COMEDI_DEVICE'],
                                              inputSubdevice=os.environ['AI_SUBDEVICE'],
                                              outputSubdevice=os.environ['AO_SUBDEVICE'],
                                              readChannel=options['ai'], writeChannel=options['ao'],
                                              inputConversionFactor=os.environ['AI_CONVERSION_FACTOR_CC'],
                                              outputConversionFactor=os.environ['AO_CONVERSION_FACTOR_CC'],
                                              inputRange=os.environ['RANGE'], reference=os.environ['GROUND_REFERENCE'],
                                              kernelFile='kernel.dat'))
    config.add_entity(lcg.entities.PID(id=2, connections=(0,1), baseline=options['amplitude'],
                                       gp=options['gp'], gi=options['gi'], gd=options['gd']))
    config.add_entity(lcg.entities.Waveform(id=3, connections=(0,2), filename='frequency.stim', units='Hz'))
    config.add_entity(lcg.entities.FrequencyEstimator(id=4, connections=(0,2), tau=options['tau'],
                                                      initial_frequency=options['min_freq']))
    if 'current_std' in options:
        config.add_entity(lcg.entities.Waveform(id=5, connections=(0,1), filename='current.stim', units='pA'))
        # Noisy current stim-file
        current = [[options['duration'],2,0,options['current_std'],options['current_tau'],0,0,0,0,0,0,1]]
        lcg.writeStimFile('current.stim',current,False)

    config.write(config_file)

    # Frequency stim-file
    frequency=[[options['duration'],-2,options['min_freq'],0,0,0,0,0,0,1,0,1],
               [options['duration'],-2,options['max_freq']-options['min_freq'],0,0,0,0,0,0,7,1,1]]
    lcg.writeStimFile('frequency.stim',frequency,False)


def main():
    opts = parseArgs()
    writeFiles(opts)
    if opts['with_kernel']:
        sub.call('lcg kernel -I ' + str(opts['ai']) + ' -O ' + str(opts['ao']), shell=True)
    sub.call(lcg.common.prog_name + ' -c ' + config_file + ' -n ' + str(opts['trials']) +
             ' -i ' + str(opts['interval']), shell=True)

if __name__ == '__main__':
    main()
