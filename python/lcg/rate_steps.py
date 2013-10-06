#!/usr/bin/env python

import sys
import os
import getopt
import subprocess as sub
import numpy as np
import lcg

gexc_file = 'gexc.stim'
ginh_file = 'ginh.stim'
config_file = 'rate_steps.xml'

def usage():
    print('\nUsage: %s [--option <value>]' % os.path.basename(sys.argv[0]))
    print('\nwhere options are:\n')
    print('     -h   Display this help message and exit.')
    print('     -n   Number of repetitions (default 100).')
    print('     -i   Interval between trials (default 2 s).')
    print('     -d   Duration of the stimulation (default 1 sec per step).')
    print('     -I   Input channel (default 0).')
    print('     -O   Output channel (default 0).')
    print('     -F   sampling frequency (default 20000).')
    print('     -k   Frequency at which a new kernel should be computed (default is just at the beginning)')
    print('     -R   Input resistance of the cell (in MOhm).')
    print('     -v   Value of voltage at which the background activity should be balanced.')
    print('     -r   Baseline firing frequency of the excitatory background population.')
    print('     -m   Fraction of the baseline firing frequency used as a modulation (default 0.1).')
    print('  --exc   Modulate the firing rate of the excitatory presynaptic population.')
    print('  --inh   Modulate the firing rate of the inhibitory presynaptic population.')
    print('')

def parseArgs():
    switches = 'hn:i:d:I:O:F:k:R:v:r:m:'
    long_switches = ['help','exc','inh']
    try:
        opts,args = getopt.getopt(sys.argv[1:],switches,long_switches)
    except getopt.GetoptError, err:
        print(str(err))
        usage()
        sys.exit(1)

    options = {'reps': 100,
               'interval': 2,             # [s]
               'step_duration': 1,        # [s]
               'kernel_frequency': None,
               'sampling_rate' : 20000,   # [Hz]
               'input_resistance': None,  # [MOhm]
               'balanced_voltage': None,  # [mV]
               'R_exc': None,             # [Hz]
               'dR': None,
               'exc': False,
               'inh': False,
               'pre': 0.25, 'post': 0.25, # [s]
               'ai': 0, 'ao': 0}

    for o,a in opts:
        if o in ('-h','--help'):
            usage()
            sys.exit(0)
        elif o == '-n':
            options['reps'] = int(a)
        elif o == '-i':
            options['interval'] = float(a)
        elif o == '-I':
            options['ai'] = int(a)
        elif o == '-O':
            options['ao'] = int(a)
        elif o == '-F':
            options['sampling_rate'] = float(a)
        elif o == '-d':
            options['step_duration'] = float(a)
        elif o == '-k':
            options['kernel_frequency'] = int(a)
        elif o == '-R':
            options['input_resistance'] = float(a)
        elif o == '-r':
            options['R_exc'] = float(a)
        elif o == '-v':
            options['balanced_voltage'] = float(a)
        elif o == '-m':
            if a.find(',') != -1:
                options['dR'] = []
                for s in a.split(','):
                    options['dR'].append(float(s))
            else:
                options['dR'] = [0, float(a)]
        elif o == '--exc':
            options['exc'] = True
        elif o == '--inh':
            options['inh'] = True

    if not options['kernel_frequency']:
        options['kernel_frequency'] = options['reps']

    if not options['exc'] and not options['inh']:
        print('You must specify whether you want to modulate the excitatory or the inhibitory rate.')
        sys.exit(1)

    if options['exc'] and options['inh']:
        print('It is not possible to modulate both excitatory and inhibitory rates.')
        sys.exit(1)

    if options['kernel_frequency'] <= 0:
        options['kernel_frequency'] = len(options['frequencies'])

    if not options['input_resistance']:
        print('You must specify the input resistance of the cell (-R switch).')
        sys.exit(1)

    if options['input_resistance'] <= 0:
        print('The input resistance of the cell must be positive.')
        sys.exit(1)
            
    if not options['R_exc']:
        print('You must specify the firing frequency of the background excitatory population (-r switch).')
        sys.exit(1)
                
    if options['R_exc'] <= 0:
        print('The firing frequency of the background population must be positive.')
        sys.exit(1)
            
    if not options['balanced_voltage']:
        print('You must specify the balanced voltage (-v switch)')
        sys.exit(1)

    if options['balanced_voltage'] >= 0:
        print('The balanced voltage must be negative.')
        sys.exit(1)

    if len(options['dR']) == 1:
        print('You must specify the rate steps of the background population.')
        sys.exit(1)

    return options

def writeConfigurationFile(options):
    config = lcg.XMLConfigurationFile(options['sampling_rate'],len(options['dR'])*options['step_duration']+options['pre']+options['post'])
    config.add_entity(lcg.entities.H5Recorder(id=0, connections=(), compress=True))
    config.add_entity(lcg.entities.RealNeuron(id=1, connections=(0), spikeThreshold=-20, V0=-65, deviceFile=os.environ['COMEDI_DEVICE'],
                                              inputSubdevice=os.environ['AI_SUBDEVICE'],
                                              outputSubdevice=os.environ['AO_SUBDEVICE'],
                                              readChannel=options['ai'], writeChannel=options['ao'],
                                              inputConversionFactor=os.environ['AI_CONVERSION_FACTOR'],
                                              outputConversionFactor=os.environ['AO_CONVERSION_FACTOR'],
                                              inputRange=os.environ['RANGE'], reference=os.environ['GROUND_REFERENCE'],
                                              kernelFile='kernel.dat'))
    config.add_entity(lcg.entities.Waveform(id=2, connections=(0,4), filename=gexc_file, units='nS'))
    config.add_entity(lcg.entities.Waveform(id=3, connections=(0,5), filename=ginh_file, units='nS'))
    config.add_entity(lcg.entities.ConductanceStimulus(id=4, connections=(1), E=0))
    config.add_entity(lcg.entities.ConductanceStimulus(id=5, connections=(1), E=-80))
    config.write(config_file)

def main():
    opts = parseArgs()
    writeConfigurationFile(opts)

    ratio = lcg.computeRatesRatio(Vm=opts['balanced_voltage'], Rin=opts['input_resistance'])
    Gm_exc,Gm_inh,Gs_exc,Gs_inh = lcg.computeSynapticBackgroundCoefficients(ratio,
                                                                            R_exc=opts['R_exc'],
                                                                            Rin=opts['input_resistance'])
    
    gexc = [[opts['pre'],1,0,0,0,0,0,0,0,0,0,1]]
    ginh = [[opts['pre'],1,0,0,0,0,0,0,0,0,0,1]]
    for dr in opts['dR']:
        if opts['exc']:
            gm = Gm_exc * (1+dr)
            gs = Gs_exc * np.sqrt(1+dr)
            gexc.append([opts['step_duration'], 2, gm, gs, 5, Gm_exc, 0, 0, int(np.random.uniform(high=10000)), 0, 0, 1])
            ginh.append([opts['step_duration'], 2, Gm_inh, Gs_inh, 10, Gm_inh, 0, 0, int(np.random.uniform(high=10000)), 0, 0, 1])
        else:
            gm = Gm_inh * (1+dr)
            gs = Gs_inh * np.sqrt(1+dr)
            gexc.append([opts['step_duration'], 2, Gm_exc, Gs_exc, 5, 0, 0, 0, int(np.random.uniform(high=10000)), 0, 0, 1])
            ginh.append([opts['step_duration'], 2, gm, gs, 10, Gm_inh, 0, 0, int(np.random.uniform(high=10000)), 0, 0, 1])
    gexc.append([opts['post'],1,0,0,0,0,0,0,0,0,0,1])
    ginh.append([opts['post'],1,0,0,0,0,0,0,0,0,0,1])
    lcg.writeStimFile(gexc_file, gexc, False)
    lcg.writeStimFile(ginh_file, ginh, False)
    
    while opts['reps'] > 0:
        reps = min(opts['reps'],opts['kernel_frequency'])
        sub.call('lcg kernel -I ' + str(opts['ai']) + ' -O ' + str(opts['ao']), shell=True)
        sub.call(lcg.common.prog_name + ' -c ' + config_file + ' -n ' + str(reps) + ' -i ' + str(opts['interval']), shell=True)
        opts['reps'] -= reps
        if opts['reps'] > 0:
            pass
            sub.call(['sleep', str(opts['interval'])])

if __name__ == '__main__':
    main()

