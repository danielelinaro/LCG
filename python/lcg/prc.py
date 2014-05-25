#!/usr/bin/env python
# Script to compute the PRC using LCG
#

import os
import sys
import numpy as np
import getopt
import subprocess as sub
import lcg
import time
from lcg.entities import (H5Recorder, LIFNeuron, 
                          RealNeuron, Waveform, 
                          Constant, ConstantFromFile,
                          SobolDelay,VariableDelayConnection,
                          EventCounter, FrequencyEstimator, 
                          PeriodicTrigger, PID)

usage = '''Usage: {0} mode [option <value>]
    Script to compute the PRC using direct or indirect methods.
    The modes are:
        steps - current steps are used to evoke repetitive firing
                this is necessary when the cell does not fire spontaneously
        fclamp - a PID controller is used to keep a constant firing rate
        fclamp-sobol - the fclamp protocol is combined with the sobol sequence
                       allowing to optimally sample the phase space.
        noise - a noisy current is injected to compute the PRC indirectly

    General options:
        -n number of repetitions (trials)
        -i interval between repetitions
        -I input channel
        -O output channel
        -F sampling frequency ({1} default)
        -H baseline current
        --dryrun
        --model

    Direct PRC options:
        -a amplitude of the perturbation (150pA default; square wave)
        -d duration of the perturbation (1ms default)
        -f perturbation frequency (2.5 Hz or number (6) of spikes in Sobol mode)
        STEPS mode:
            -A amplitude of the step (default 200pA)
            -D duration of the step (3s)
            -S perturbation delay (1s default)
        FCLAMP mode:
            -P proportional gain
            -G integrative gain
            -T Frequency estimator time constant (0.1)
            -t Target firing frequency (30Hz)
            -D Duration of the trial (48s)
        FCLAMP-SOBOL mode:
            -D Number of stimulations per trial
            -S offset of the sobol sequence so that we don't start 
               sampling always the same points
    
    Indirect PRC; NOISE mode options:
            -D duration of the noisy waveform (default 30s)
            -A amplitude of the noise (0pA)
            -S standard deviation of the noise (50pA)
            -T time constant of the noise (20ms)
    
Examples:
lcg-prc fclamp-sobol -D 20 -f 20 -n 1 --model --no-kernel -H 220 -t 20 -n 10
lcg-prc fclamp -D 20 -f 2.5 -n 10 --model --no-kernel -H 220 -t 40 -n 10
lcg-prc noise -n 2 -t 60 -A 200  --model --no-kernel

'''.format(os.path.basename(sys.argv[0]), os.environ['SAMPLING_RATE'])


def parse_prc_options(mode,opts):
    print opts
    env = lambda x:os.environ[x]
    defaults = {'ai':env('AI_CHANNEL'),
                'ao':env('AI_CHANNEL'),
                'holding':0.0,
                'ntrials':10,
                'tpre':1,
                'tpost':1,
                'intreps':0.05,
                'srate':env('SAMPLING_RATE'),
                'dryrun':False,
                'kernel':True,
                'model':False}
    if mode in ('steps','fclamp','fclamp-sobol'):
        defaults.update({'pert_amp':150,
                         'pert_dur':1,
                         'pert_waveform':'dc',
                         'pert_freq':2.5,
                         })
        if mode == 'steps':
            defaults.update({'step_amp':200,
                             'step_dur':3,
                             'intreps':5,
                             'ntrials':150,
                             'step_pdelay':1})

        elif 'fclamp' in mode: 
            defaults.update({'gi':0.001,
                             'gp':0.1,
                             'gd':0,
                             'Ftau':0.1,
                             'pert_freq':6,
                             'trial_dur':48,
                             'target_freq':30,
                             'ntrials':10,
                             'intreps':0})
        else:
            defaults.update({'sobol_offset':0})
    else:
        defaults.update({'ou_amp':0,
                         'ou_std':50,
                         'ou_tau':20,
                         'ou_dur':30,
                         'ntrials':1})
    options = defaults.copy()
    for o,a in opts:
        if o == '-I':
            options['ai'] = [int(i) for i in a.split(',')]
        elif o == '-O':
            options['ao'] = [int(i) for i in a.split(',')]
        elif o == '-F':
            options['srate'] = [float(a)]
        elif o == '-H':
            options['holding'] = float(a)
        elif o == '-i':
            options['intreps'] = float(a) 
        elif o == '-n':
            options['ntrials'] = int(a) 
        elif o == '-a':
            options['pert_amp'] = float(a)
        elif o == '-d':
            options['pert_dur'] = float(a)
        elif o == '-f':
            options['pert_freq'] = float(a)
        elif o == '-t':
            options['target_freq'] = float(a)
        elif o == '-A':
            if mode == 'steps':
                options['step_amp'] = float(a)
            else :
                options['ou_amp'] = float(a)
        elif o == '-D':
            if mode == 'steps':
                options['step_dur'] = float(a)
            elif 'fclamp' in mode:
                options['trial_dur'] = float(a)
            else:
                options['ou_dur'] = float(a)
        elif o == '-S':
            if mode == 'steps':
                options['step_pdelay'] = float(a)
            elif mode == 'sobol': 
                options['sobol_offset'] = float(a)
            else:
                options['ou_std'] = float(a)
        elif o == '-T':
            if 'fclamp' in mode: 
                options['Ftau'] = float(a)
            else:
                options['ou_tau'] = float(a)
        elif o == '-P':
            options['gp'] = float(a)
        elif o == '-G':
            options['gi'] = float(a)
        elif o == '--model':
            options['model'] = True
        elif o == '--no-kernel':
            options['kernel'] = False
        elif o == '--dryrun':
            options['dryrun'] = True
        elif o in ['help','--help','-h']:
            print(usage)
            sys.exit(1)
        else:
            print('Unknown option...')
            print(usage)
            sys.exit(1)

    return options

modes = ['steps','fclamp','fclamp-sobol','noise']
switches = 'hn:i:I:O:F:H:a:d:f:t:A:D:P:G:T:S:'
long_switches = ['help','no-kernel','dryrun','model']

def build_steps_config(opts, config,lastValueFilename):
    # Insert periodic trigger
    baselineName = 'baseline.stim'
    config.add_entity(PeriodicTrigger(3, (2), 
                                      delay = opts['step_pdelay'],
                                      tend = opts['step_dur'] + opts['tpre'],
                                      frequency = opts['pert_freq']))
    config.add_entity(Waveform(4,(0,1),filename=baselineName, units = 'pA'))

    sys.argv = ['lcg-stimgen','-o',baselineName,
                'dc', '-d',str(opts['tpre']),'--','0',
                'dc', '-d',str(opts['step_dur']),'--',str(opts['step_amp']),
                'dc', '-d',str(opts['tpost']),'--','0']
    lcg.stimgen.main()

    # Holding current (use a ConstantFromFile)
    config.add_entity(ConstantFromFile(id=len(config._entities),
                               connections=[0,1], 
                               filename=lastValueFilename,
                               units='pA'))

    config.set_tend(opts['tpre']+opts['step_dur']+opts['tpost'])

def build_fclamp_config(opts, config,lastValueFilename):
    config.add_entity(PeriodicTrigger(3, (2), 
                                      frequency = opts['pert_freq']))
    config.add_entity(PID(4,(0,1),
                          baseline=0.0,
                          gi=opts['gi'],
                          gp=opts['gp'],
                          gd=opts['gd']))
    config.add_entity(Constant(id=5,
                               connections=[0,4],
                               value=opts['target_freq'],
                               units='Hz'))
    config.add_entity(FrequencyEstimator(6,(0,4),
                                         tau = opts['Ftau'],
                                         initial_frequency= opts['target_freq']))
    config.add_entity(ConstantFromFile(id=len(config._entities),
                               connections=[0,1], 
                               filename=lastValueFilename,
                               units='pA'))
    
    # Fix RealNeuronConnections
    config._add_connections(config._entities[1],[6])
    config.set_tend(opts['trial_dur'])

def build_fclamp_sobol_config(opts, config, lastValueFilename):
    config.add_entity(PID(3,(0,1),
                          baseline=0.0,
                          gi=opts['gi'],
                          gp=opts['gp'],
                          gd=opts['gd']))
    config.add_entity(Constant(id=4,
                               connections=[0,3],
                               value=opts['target_freq'],
                               units='Hz'))
    config.add_entity(FrequencyEstimator(5,(0,3,6),
                                         tau = opts['Ftau'],
                                         initial_frequency= opts['target_freq']))
    config.add_entity(SobolDelay(6,connections=(7)))
    config.add_entity(VariableDelayConnection(7,connections=[2]))
    config.add_entity(EventCounter(8,connections=[7],
                                   max_count=2,auto_reset=False))
    config.add_entity(EventCounter(9,connections=[3,5],
                                   max_count=2,event_to_send='TOGGLE',
                                   auto_reset=False))
    config.add_entity(EventCounter(10,connections=[3,5],
                                   max_count=4,event_to_send='TOGGLE',
                                   auto_reset=False))
    config.add_entity(EventCounter(11,connections=[8,9,10,12],
                                   max_count=opts['pert_freq'],event_to_send='RESET',
                                   auto_reset=True))
    config.add_entity(EventCounter(12,connections=[],
                                   max_count=opts['trial_dur'],event_to_count='RESET',
                                   event_to_send='STOPRUN',
                                   auto_reset=True))
    
    config.add_entity(ConstantFromFile(id=len(config._entities),
                               connections=[0,1], 
                               filename=lastValueFilename,
                               units='pA'))
    
    # Fix RealNeuronConnections
    config._add_connections(config._entities[1],[5,8,9,10,11])
    config.set_tend(opts['trial_dur'])

def build_noise_config(opts, config, waveformName,lastValueFilename):
    config.add_entity(Waveform(id=2,
                               connections=[0,1], 
                               filename=waveformName,
                               units='pA'))
    sys.argv = ['lcg-stimgen','-o',waveformName,
                'dc', '-d',str(opts['tpre']),'--','0',
                'ou', '-d',str(opts['ou_dur']),'--',
                str(opts['ou_amp']),str(opts['ou_std']),str(opts['ou_tau']),
                'dc', '-d',str(opts['tpost']),'--','0']
    lcg.stimgen.main()

    config.add_entity(ConstantFromFile(id=len(config._entities),
                               connections=[0,1], 
                               filename=lastValueFilename,
                               units='pA'))    
    config.set_tend(opts['tpre']+opts['ou_dur']+opts['tpost'])
    
def run_kernel(opts):
    kernel_cmd = ('lcg-kernel -I {0} -O {1} -F {2} -H {3}'.format(opts['ai'],
                                                                  opts['ao'],
                                                                  opts['srate'],
                                                                  opts['holding']))
    print('''
Going to run the kernel protocol. 
The cell should not be firing spontaneous. 
-> Use the amplifier to hyperpolarize the cell if necessary and press [ENTER].
''')
    raw_input()
    if opts['dryrun'] or not opts['model'] is None:
        print(kernel_cmd)
    else:
        sub.call(kernel_cmd,shell=True)
    print('''
-> Release hyperpolarization and press [ENTER].
''')
    raw_input()
    
def insert_model(config,holding,lastValueFilename):

    config.add_entity(LIFNeuron(1, (0), C=0.08, tau=0.0075,
                                tarp=0.0014, Er=-65.2,
                                E0=-70, Vth=-50, Iext=0,
                                holdLastValue=True,
                                holdLastValueFilename=lastValueFilename))
def hold_cell(opts,lastValueFilename):
    print('Holding the cell with {0} pA'.format(opts['holding']))
    if opts['model']:
        with open(lastValueFilename,'w') as fd:
            fd.write('%e'%(opts['holding']))
        return ''
    else:
        # Then use a RealNeuron to inject into the cell
        tmp = lcg.XMLConfigurationFile(opts['srate'], 0.01)

        insertRealNeuron(tmp, opts, [], lastValueFilename)
        tmp.add_entity(Constant(id=2,
                                   connections=[1],
                                   value=opts['holding'],
                                   units='pA'))
        tmpName = 'holdNeuron.xml'
        tmp.write(tmpName)
        return 'lcg-experiment -c {0}'.format(tmpName)

def insertRealNeuron(config, opts, connections,lastValueFilename):
    config.add_entity(RealNeuron(id=1,
                                 connections=connections,
                                 spikeThreshold=-20,
                                 V0=-65,
                                 deviceFile=env('COMEDI_DEVICE'),
                                 inputSubdevice=env('AI_SUBDEVICE'),
                                 outputSubdevice=env('AO_SUBDEVICE'),
                                 readChannel=opts['ai'],
                                 writeChannel=opts['ao'],
                                 inputConversionFactor=env('AI_CONVERSION_FACTOR_CC'),
                                 outputConversionFactor=env('AO_CONVERSION_FACTOR_CC'),
                                 inputRange=env('RANGE'),
                                 reference=env('GROUND_REFERENCE'),
                                 holdLastValue=True,
                                 holdLastValueFilename=lastValueFilename,
                                 kernelFile='kernel.dat'))


def main():
    if len(sys.argv) < 2 or sys.argv[1] in ('-h','--help','help'):
        print(usage)
        sys.exit(0)
    mode = sys.argv[1]
    if not mode in modes:
        print('''Unknown mode: {0}; try instead 
one of the following: {1}'''.format(mode,
                                    ', '.join(modes)))
        sys.exit(1)
    try:
        opts,args = getopt.getopt(sys.argv[2:], switches,long_switches)
    except getopt.GetoptError, err:
        print(err)
        print(usage)
        sys.exit(1)
    opts = parse_prc_options(mode,opts)

    configName = 'prc.xml'
    waveformName = 'prc.stim'
    lastValueFilename = 'prc.lastValue'
    hold_time = 3
    duration = 0

    config = lcg.XMLConfigurationFile(opts['srate'], duration)
    config.add_entity(H5Recorder(id=0, 
                                 connections=[],
                                 compress=True))
    if opts['model']:
        insert_model(config,opts['holding'],lastValueFilename)
    else:
        insertRealNeuron(config, opts, connections, lastValueFilename)
# Perturbation waveform
    if not mode == 'noise':
        config.add_entity(Waveform(id=2,
                                   connections=[0,1], 
                                   filename=waveformName,
                                   triggered=True,
                                   units='pA'))
        sys.argv = ['lcg-stimgen','-o',waveformName,
                    opts['pert_waveform'],
                    '-d',str(opts['pert_dur']*1e-3),
                    '--',str(opts['pert_amp'])]
        lcg.stimgen.main()
    if mode == 'steps':
        build_steps_config(opts, config,lastValueFilename)
    elif mode == 'fclamp':
        build_fclamp_config(opts, config,lastValueFilename)
    elif mode == 'fclamp-sobol':
        build_fclamp_sobol_config(opts, config,lastValueFilename)        
    else: # Then it is for the WSTA
        build_noise_config(opts, config, waveformName,lastValueFilename)        

    # Run or dry-run?
    trial_cmd = 'lcg-experiment -c {0}'.format(configName)
    if opts['dryrun']:
        run = lambda x: sys.stdout.write(x+'\n')
    else:
        run = lambda x: sub.call(x, shell=True)

    # Initialization
    if opts['kernel']:
        run_kernel(opts)
    run(hold_cell(opts,lastValueFilename))
    if not opts['dryrun']:
        print('Sleeping {0}...'.format(hold_time))
        time.sleep(hold_time)
    print('Going to run {0} trials'.format(opts['ntrials']))
    
    for ii in range(opts['ntrials']):
        config.write(configName)
        run(trial_cmd)
        time.sleep(opts['intreps'])

if __name__ == '__main__':
    main()
