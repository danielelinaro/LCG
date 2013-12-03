#!/usr/bin/env python

import os
import sys
import numpy as np
import lcg
import getopt
import subprocess as sub

config_file = 'fclamp.xml'

def usage():
    print('\nUsage: %s [option <value>]' % os.path.basename(sys.argv[0]))
    print('\nwhere options are:\n')
    print('     -f        Target frequency.')
    print('     -T        Stimulation duration (default 30 s).')
    print('     -t        Time constant of the frequency estimator (default 1 s).')
    print('     -a        Amplitude of the initially injected current (default 0 pA).')
    print('     -p        Proportional gain of the controller (default 0.05).')
    print('     -i        Integral gain of the controller (default 1).')
    print('     -d        Derivative gain of the controller (default 0).')
    print('     -n        Number of repetitions (default 1).')
    print('     -w        Interval between repetitions (default 30 s).')
    print('     -I        Input channel (default 0).')
    print('     -O        Output channel (default 0).')
    print('  --with-bg    Add background synaptic noise (default is no).')
    print('\nThe following options are valid only if the --with-bg switch is used:\n')
    print('     -R        Input resistance of the cell (in MOhm).')
    print('     -v        Value of voltage at which the background activity should be balanced.')
    print('     -F        Firing frequency of the excitatory background population.')
    print('')

def writeFiles(options):
    stim_file = 'frequency.stim'
    config = lcg.XMLConfigurationFile(options['sampling_rate'],options['duration'])
    config.add_entity(lcg.entities.H5Recorder(id=0, connections=(), compress=True))
    config.add_entity(lcg.entities.RealNeuron(id=1, connections=(0,4), spikeThreshold=-20, V0=-65, deviceFile=os.environ['COMEDI_DEVICE'],
                                              inputSubdevice=os.environ['AI_SUBDEVICE'],
                                              outputSubdevice=os.environ['AO_SUBDEVICE'],
                                              readChannel=options['ai'], writeChannel=options['ao'],
                                              inputConversionFactor=os.environ['AI_CONVERSION_FACTOR'],
                                              outputConversionFactor=os.environ['AO_CONVERSION_FACTOR'],
                                              inputRange=os.environ['RANGE'], reference=os.environ['GROUND_REFERENCE'],
                                              kernelFile='kernel.dat'))
    config.add_entity(lcg.entities.PID(id=2, connections=(0,1), baseline=options['I0'], gp=options['gp'], gi=options['gi'], gd=options['gd']))
    config.add_entity(lcg.entities.Waveform(id=3, connections=(0,2), filename=stim_file, units='Hz'))
    config.add_entity(lcg.entities.FrequencyEstimator(id=4, connections=(0,2), tau=options['tau'], initial_frequency=options['target']/2))
    if 'Gm_exc' in options and 'Gm_inh' in options and 'Gs_exc' in options and 'Gs_inh' in options:
        gexc_stim_file = 'gexc.stim'
        ginh_stim_file = 'ginh.stim'
        config.add_entity(lcg.entities.Waveform(id=5, connections=(0,7), filename=gexc_stim_file, units='nS'))
        config.add_entity(lcg.entities.Waveform(id=6, connections=(0,8), filename=ginh_stim_file, units='nS'))
        config.add_entity(lcg.entities.ConductanceStimulus(id=7, connections=(1), E=0))
        config.add_entity(lcg.entities.ConductanceStimulus(id=8, connections=(1), E=-80))
        lcg.writeStimFile(gexc_stim_file, [options['duration'],2,options['Gm_exc'],options['Gs_exc'],5,0,0,1,int(np.random.uniform(high=10000)),0,0,1], False)
        lcg.writeStimFile(ginh_stim_file, [options['duration'],2,options['Gm_inh'],options['Gs_inh'],10,0,0,1,int(np.random.uniform(high=10000)),0,0,1], False)
    config.write(config_file)
    lcg.writeStimFile(stim_file, [options['duration'],1,options['target'],0,0,0,0,0,0,0,0,1], False)

def main():
    try:
        opts,args = getopt.getopt(sys.argv[1:], 'hf:T:t:a:n:p:i:d:t:w:I:O:R:F:v:', ['help','with-bg'])
    except getopt.GetoptError, err:
        print(str(err))
        usage()
        sys.exit(1)
        
    nreps = 1
    interval = 30                  # [s]
    options = {
        'ao': int(os.environ['AI_CHANNEL']), 'ai': int(os.environ['AO_CHANNEL']),
        'gp': 0.05, 'gi': 1., 'gd': 0.,
        'sampling_rate': float(os.environ['SAMPLING_RATE']),   # [Hz]
        'duration': 30.,           # [s]
        'tau': 1.,                 # [s]
        'I0': 0.,                  # [pA]
        'target': None,            # [Hz]
        'input_resistance': None,  # [MOhm]
        'balanced_voltage': None,  # [mV]
        'R_exc': None              # [Hz]
        }

    for o,a in opts:
        if o in ['-h','--help']:
            usage()
            sys.exit(0)
        elif o == '-n':
            nreps = int(a)
        elif o == '-w':
            interval = float(a)
        elif o == '-f':
            options['target'] = float(a)
        elif o == '-T':
            options['duration'] = float(a)
        elif o == '-t':
            options['tau'] = float(a)
        elif o == '-a':
            options['I0'] = float(a)
        elif o == '-I':
            options['ai'] = int(a)
        elif o == '-O':
            options['ao'] = int(a)
        elif o == '-R':
            options['input_resistance'] = float(a)
        elif o == '-F':
            options['R_exc'] = float(a)
        elif o == '-v':
            options['balanced_voltage'] = float(a)
        elif o == '-p':
            options['gp'] = float(a)
        elif o == '-i':
            options['gi'] = float(a)
        elif o == '-d':
            options['gd'] = float(a)
            
    if options['target'] is None:
        print('You must specify specify the target frequency (-f switch).')
        sys.exit(1)
        
    if options['input_resistance'] is not None or options['R_exc'] is not None or options['balanced_voltage'] is not None:
        if options['input_resistance'] is None or options['R_exc'] is None or options['balanced_voltage'] is None:
            print('To inject a noisy background input, you must specify:')
            print('  1. The input resistance of the cell (-R switch).')
            print('  2. The firing frequency of the background excitatory population (-F switch).')
            print('  3. The balanced voltage (-v switch).')
            sys.exit(1)
        ratio = lcg.computeRatesRatio(options['balanced_voltage'], options['input_resistance'])
        options['Gm_exc'],options['Gm_inh'],options['Gs_exc'],options['Gs_inh'] = \
                       lcg.computeSynapticBackgroundCoefficients(ratio, options['R_exc'], options['input_resistance'])

    writeFiles(options)

    sub.call('lcg kernel -I ' + str(options['ai']) + ' -O ' + str(options['ao']), shell=True)
    sub.call(lcg.common.prog_name + ' -c ' + config_file + ' -n ' + str(nreps) + ' -i ' + str(interval), shell=True)

if __name__ == '__main__':
    main()


