#!/usr/bin/env python

import sys
import os
import getopt
import subprocess as sub
import numpy as np
import lcg
import copy

current_file = 'current.stim'
modulation_file = 'mod.stim'
gexc_file = 'gexc.stim'
ginh_file = 'ginh.stim'
config_file = 'sinusoids.xml'
random_seed = 5061983
frequency_value = 7051983

switches = 'f:n:i:d:I:O:k:R:r:F:v:a:s:t:m:'
long_switches = ['with-bg','exc','inh','no-kernel','separate']

def usage():
    print('\nUsage: %s <mode> [--option <value>]' % os.path.basename(sys.argv[0]))
    print('\nThe working modes are:\n')
    print('    current      use a sinusoidally modulated current.')
    print('    conductance  use sinusoidally modulated conductances.')
    print('\nThe global options are:\n')
    print('     -h   Display this help message and exit.')
    print('     -f   Frequencies of the sinusoids (comma-separated values).')
    print('     -n   Number of repetitions (default 1).')
    print('     -i   Interval between trials (default 60 s).')
    print('     -d   Duration of the stimulation (default 30 sec).')
    print('     -I   Input channel (default 0).')
    print('     -O   Output channel (default 0).')
    print('     -F   sampling frequency (default 20000).')
    print('     -k   Frequency at which a new kernel should be computed')
    print('          (the default is at the beginning of each batch of frequencies).')
    print(' --no-kernel  Do not compute the kernel.')

    print('\nThe following options are valid in the "current" working mode:\n')
    print('     -a   Mean of the noisy component of the current (default 0 pA).')
    print('     -s   Standard deviation of the noisy component of the current (default 0 pA).')
    print('     -t   Time constant of the noisy component of the current (default 0 ms).')
    print('     -m   Amplitude of the modulating current (default 30 pA).')

    print('\nAdditionally, if the --with-bg option is specified, the following')
    print('options are accepted:\n')
    print('     -r   Firing frequency of the excitatory background population.')
    print('     -R   Input resistance of the cell (in MOhm).')
    print('     -v   Value of voltage at which the background activity should be balanced.')

    print('\nThe following options are valid in the "conductance" working mode:\n')
    print('     -R   Input resistance of the cell (in MOhm).')
    print('     -v   Value of voltage at which the background activity should be balanced.')
    print('     -r   Baseline firing frequency of the excitatory background population.')
    print('     -m   Fraction of the baseline firing frequency used as a modulation (default 0.1).')
    print('  --exc   Modulate the firing rate of the excitatory presynaptic population.')
    print('  --inh   Modulate the firing rate of the inhibitory presynaptic population.')
    print('  --separate  Injects current waveform in a separate channel (defined as the second defined in -O ) ')
    print('')

def parseGlobalArgs():
    try:
        opts,args = getopt.getopt(sys.argv[2:],switches,long_switches)
    except getopt.GetoptError, err:
        print(str(err))
        usage()
        sys.exit(1)
        
    options = {'frequencies': [1,5,10,20,50,100,200,300,600,1000], # [Hz]
               'reps': 1,
               'interval': 60,   # [s]
               'duration': 30,   # [s]
               'kernel_frequency': 0,
               'compute_kernel': True,
               'sampling_rate' : 20000,  # [Hz]
               'ai': 0, 'ao': 0}

    for o,a in opts:
        if o == '-f':
            options['frequencies'] = []
            for f in a.split(','):
                options['frequencies'].append(float(f))
        elif o == '-n':
            options['reps'] = int(a)
        elif o == '-i':
            options['interval'] = float(a)
        elif o == '-I':
            options['ai'] = [int(i) for i in a.split(',')]
            if len(options['ai']) == 1:
                options['ai'] = options['ai'][0]
        elif o == '-O':
            options['ao'] = [int(i) for i in a.split(',')]
            if len(options['ao']) == 1:
                options['ao'] = options['ao'][0]
        elif o == '-F':
            options['sampling_rate'] = float(a)
        elif o == '-d':
            options['duration'] = float(a)
        elif o == '--no-kernel':
            options['compute_kernel'] = False
        elif o == '-k':
            options['kernel_frequency'] = int(a)

    if not options['compute_kernel'] and options['kernel_frequency'] > 0:
        print('You specified both the --no-kernel option and a kernel frequency. Since this does not make sense, I cowardly refuse to continue.')
        sys.exit(1)

    if options['kernel_frequency'] <= 0:
        options['kernel_frequency'] = len(options['frequencies'])

    return options

def parseBackgroundArgs():
    try:
        opts,args = getopt.getopt(sys.argv[2:],switches,long_switches)
    except getopt.GetoptError, err:
        print(str(err))
        usage()
        sys.exit(1)

    options = {'input_resistance': None,   # [MOhm]
               'balanced_voltage': None,   # [mV]
               'R_exc': None}              # [Hz]

    for o,a in opts:
        if o == '-R':
            options['input_resistance'] = float(a)
        elif o == '-r':
            options['R_exc'] = float(a)
        elif o == '-v':
            options['balanced_voltage'] = float(a)

    if not options['input_resistance']:
        print('You must specify the input resistance of the cell (-R switch).')
        sys.exit(1)
            
    if not options['R_exc']:
        print('You must specify the firing frequency of the background excitatory population (-r switch).')
        sys.exit(1)
                
    if not options['balanced_voltage']:
        print('You must specify the balanced voltage (-v switch)')
        sys.exit(1)

    return options

def parseCurrentModeArgs():
    try:
        opts,args = getopt.getopt(sys.argv[2:],switches,long_switches)
    except getopt.GetoptError, err:
        print(str(err))
        usage()
        sys.exit(1)

    options = {'with_bg': False,
               'mean': 0,                # [pA]
               'std': 0,                 # [pA]
               'tau': 0,                 # [ms]
               'I_modul': 30,            # [pA]
               'separate':False}
    for o,a in opts:
        if o == '--with-bg':
            options['with_bg'] = True
            options = dict(parseBackgroundArgs(), **options)
        elif o == '-a':
            options['mean'] = float(a)
        elif o == '-s':
            options['std'] = float(a)
        elif o == '-t':
            options['tau'] = float(a)
        elif o == '-m':
            options['I_modul'] = float(a)
        elif o == '--separate':
            options['separate'] = True

    if options['std'] < 0:
        print('The standard deviation of the noisy current must be positive.')
        sys.exit(1)

    if options['tau'] < 0:
        print('The standard deviation of the noisy current must be positive.')
        sys.exit(1)

    if options['tau'] == 0 and options['std'] > 0:
        print('If you specify a value for the standard deviation, you must also give a ' + 
              'value for the time constant of the noisy current.')
        sys.exit(1)

    if options['I_modul'] < 0:
        print('The amplitude of the modulating current must be positive.')
        sys.exit(1)

    return options

def parseConductanceModeArgs():
    try:
        opts,args = getopt.getopt(sys.argv[2:],switches,long_switches)
    except getopt.GetoptError, err:
        print(str(err))
        usage()
        sys.exit(1)

    options = parseBackgroundArgs()
    options['dR'] =  0.1      # [fraction]
    options['exc'] = False
    options['inh'] = False

    for o,a in opts:
        if o == '-m':
            options['dR'] = float(a)
        elif o == '--exc':
            options['exc'] = True
        elif o == '--inh':
            options['inh'] = True

    if options['dR'] < 0:
        print('The modulating fraction must be positive.')
        sys.exit(1)

    if not options['exc'] and not options['inh']:
        print('You must specify whether you want to modulate the excitatory or the inhibitory rate, or both.')
        sys.exit(0)

    return options

def writeConfigurationFile(options):
    config = lcg.XMLConfigurationFile(options['sampling_rate'],options['duration'])
    config.add_entity(lcg.entities.H5Recorder(id=0, connections=(), compress=True))
    config.add_entity(lcg.entities.RealNeuron(id=1, connections=(0), spikeThreshold=-20, V0=-65, deviceFile=os.environ['COMEDI_DEVICE'],
                                              inputSubdevice=os.environ['AI_SUBDEVICE'],
                                              outputSubdevice=os.environ['AO_SUBDEVICE'],
                                              readChannel=options['ai'], writeChannel=options['ao'],
                                              inputConversionFactor=os.environ['AI_CONVERSION_FACTOR'],
                                              outputConversionFactor=os.environ['AO_CONVERSION_FACTOR'],
                                              inputRange=os.environ['RANGE'], reference=os.environ['GROUND_REFERENCE'],
                                              kernelFile='kernel.dat'))
    config.add_entity(lcg.entities.Waveform(id=2, connections=(0,1), filename=current_file, units='pA'))
    config.add_entity(lcg.entities.Waveform(id=3, connections=(0,5), filename=gexc_file, units='nS'))
    config.add_entity(lcg.entities.Waveform(id=4, connections=(0,6), filename=ginh_file, units='nS'))
    config.add_entity(lcg.entities.ConductanceStimulus(id=5, connections=(1), E=0))
    config.add_entity(lcg.entities.ConductanceStimulus(id=6, connections=(1), E=-80))
    config.write(config_file)

def replaceValue(stimulus, old_value, new_value=None):
    idx = np.nonzero(np.array(stimulus) == old_value)
    for r in idx[0]:
        for c in idx[1]:
            if new_value:
                stimulus[r][c] = new_value
            else:
                stimulus[r][c] = int(np.random.uniform(high=10000))

def createSinusoidallyModOU(f, r0, dr, Rin, tau, dur, type, seed):
    if type == 'exc':
        g = 0.02/Rin*1e3   # [nS]
    elif type == 'inh':
        g = 0.06/Rin*1e3   # [nS]
    else:
        print("Unknown type [%s]: allowed values are 'exc' or 'inh'." % type)
        return -1
    tau_ms = tau   # [ms]
    tau = tau*1e-3 # [s]
    G = [[2.61,1,0,0,0,0,0,0,0,0,0,1], # "preamble"
         [dur,-3,0,1,tau_ms,0,0,1,seed,2,0,1], # OU
         [0,-3,g**2*tau/2*dr,f,0,g**2*tau/2*r0,0,0,0,3,2,0.5], # sine that sets the variance
         [0,-3,g*tau*dr,f,0,g*tau*r0,0,0,0,3,1,1], # sine that sets the mean
         [1,1,0,0,0,0,0,0,0,0,0,1]] # one second at the end
    return G

def main():
    mode = None

    if len(sys.argv) < 2 or sys.argv[1] in ('-h','--help','help'):
        usage()
        sys.exit(0)

    mode = sys.argv[1]
    if mode != 'current' and mode != 'conductance':
        print('Unknown working mode: [%s].' % sys.argv[1])
        sys.exit(1)

    opts = parseGlobalArgs()

    if mode == 'current':
        opts = dict(parseCurrentModeArgs(), **opts)
    else:
        opts = dict(parseConductanceModeArgs(), **opts)

    if (mode == 'current' and opts['with_bg']) or mode == 'conductance':
        ratio = lcg.computeRatesRatio(Vm=opts['balanced_voltage'], Rin=opts['input_resistance'])
        Gm_exc,Gm_inh,Gs_exc,Gs_inh = lcg.computeSynapticBackgroundCoefficients(ratio, R_exc=opts['R_exc'], Rin=opts['input_resistance'])
    else:
        # no background conductances, it will be a current-clamp experiment
        gexc = None
        ginh = None

    if mode == 'current':
        if opts['with_bg']:
            # background conductances only
            gexc = [[2.61,1,0,0,0,0,0,0,0,0,0,1],
                    [opts['duration'],2,Gm_exc,Gs_exc,5,0,0,1,random_seed,0,0,1],
                    [1,1,0,0,0,0,0,0,0,0,0,1]]
            ginh = [[2.61,1,0,0,0,0,0,0,0,0,0,1],
                    [opts['duration'],2,Gm_inh,Gs_inh,10,0,0,1,random_seed,0,0,1],
                    [1,1,0,0,0,0,0,0,0,0,0,1]]
            
        if opts['mean'] == 0 and opts['std'] == 0 and opts['tau'] == 0:
            # sinusoidal modulating current only
            current = [[opts['duration'],3,opts['I_modul'],frequency_value,0,0,0,0,0,0,0,1],
                       [1,1,0,0,0,0,0,0,0,0,0,1]]
            if opts['separate']:
                current = [[opts['duration'],1,0,0,0,0,0,1,random_seed,0,0,1], # OU current
                           [1,1,0,0,0,0,0,0,0,0,0,1]]
                modulation = [[opts['duration'],3,opts['I_modul'],frequency_value,0,0,0,0,0,0,0,1],
                              [1,1,0,0,0,0,0,0,0,0,0,1]]
        elif opts['tau'] == 0:
            # sinusoid on top of a DC current
            current = [[opts['duration'],-2,opts['mean'],0,0,0,0,0,0,1,0,1],
                       [0,-2,opts['I_modul'],frequency_value,0,0,0,0,0,3,1,1],
                       [1,1,0,0,0,0,0,0,0,0,0,1]]
            if opts['separate']:
                current = [[opts['duration'],1,opts['mean'],0,0,0,0,0,random_seed,0,0,1], # OU current
                           [1,1,0,0,0,0,0,0,0,0,0,1]]
                modulation = [[opts['duration'],3,opts['I_modul'],frequency_value,0,0,0,0,0,0,0,1],
                              [1,1,0,0,0,0,0,0,0,0,0,1]]
                       
        else:
            # sinusoid on top of a noisy (OU) current
            current = [[opts['duration'],-2,opts['mean'],opts['std'],opts['tau'],0,0,1,random_seed,2,0,1], # OU current
                       [0,-2,opts['I_modul'],frequency_value,0,0,0,0,0,3,1,1],
                       [1,1,0,0,0,0,0,0,0,0,0,1]]
            if opts['separate']:
                current = [[opts['duration'],2,opts['mean'],opts['std'],opts['tau'],0,0,1,random_seed,0,0,1], # OU current
                           [1,1,0,0,0,0,0,0,0,0,0,1]]
                modulation = [[opts['duration'],3,opts['I_modul'],frequency_value,0,0,0,0,0,0,0,1],
                              [1,1,0,0,0,0,0,0,0,0,0,1]]

    else:
        # current just for the preamble
        current = [[opts['duration']+1,1,0,0,0,0,0,0,0,0,0,1]]
        # conductances
        opts['R_inh'] = opts['R_exc']/ratio
        if opts['exc']:
            gexc = createSinusoidallyModOU(frequency_value, opts['R_exc'], opts['dR']*opts['R_exc'],
                                              opts['input_resistance'], 5, opts['duration'], 'exc', random_seed)
        else:
            gexc = [[2.61,1,0,0,0,0,0,0,0,0,0,1],
                    [opts['duration'],2,Gm_exc,Gs_exc,5,0,0,1,random_seed,0,0,1],
                    [1,1,0,0,0,0,0,0,0,0,0,1]]
        if opts['inh']:
            ginh = createSinusoidallyModOU(frequency_value, opts['R_inh'], opts['dR']*opts['R_inh'],
                                              opts['input_resistance'], 10, opts['duration'], 'inh', random_seed)
        else:
            ginh = [[2.61,1,0,0,0,0,0,0,0,0,0,1],
                    [opts['duration'],2,Gm_inh,Gs_inh,10,0,0,1,random_seed,0,0,1],
                    [1,1,0,0,0,0,0,0,0,0,0,1]]

    if gexc and ginh:
        writeConfigurationFile(opts)
        
    cnt = 0
    tot = opts['reps']*len(opts['frequencies'])
    for i in range(opts['reps']):
        np.random.shuffle(opts['frequencies'])
        for f in opts['frequencies']:
            if opts['compute_kernel'] and cnt%opts['kernel_frequency'] == 0:
                if mode  == 'current' and opts['separate']:
                    # Run the kernel for each channel.
                    if (type(opts['ao']) is int) or (type(opts['ai']) is int):
                        print('Separate only works if 2 channels are specified (use -I and -O options).\n')
                        sys.exit(1)
                    sub.call('lcg kernel -I ' + str(opts['ai'][0]) + ' -O ' + str(opts['ao'][0]) +
                             ' -F '+ str(opts['sampling_rate']) + ' --non-rt --append', shell=True)
                    sub.call('lcg kernel -I ' + str(opts['ai'][1]) + ' -O ' + str(opts['ao'][1]) +
                             ' -F '+ str(opts['sampling_rate']) + ' --non-rt --append', shell=True)
                else:
                    sub.call('lcg kernel -I ' + str(opts['ai']) + ' -O ' + str(opts['ao']) +
                             ' -F '+ str(opts['sampling_rate']), shell=True)
            cnt = cnt+1
            print('[%02d/%02d] F = %g Hz.' % (cnt,tot,f))
            
            I = copy.deepcopy(current)
            replaceValue(I, random_seed)
            replaceValue(I, frequency_value, f)
            lcg.writeStimFile(current_file, I, addDefaultPreamble=True)
            if mode == 'current' and opts['separate']:
                I = copy.deepcopy(modulation)
                replaceValue(I, frequency_value, f)
                lcg.writeStimFile(modulation_file, I, addDefaultPreamble=True)
            if gexc and ginh:
                G = copy.deepcopy(gexc)
                replaceValue(G, random_seed)
                replaceValue(G, frequency_value, f)
                lcg.writeStimFile(gexc_file, G)
                G = copy.deepcopy(ginh)
                replaceValue(G, random_seed)
                replaceValue(G, frequency_value, f)
                lcg.writeStimFile(ginh_file, G)
                sub.call(lcg.common.prog_name + ' -V 3 -c ' + config_file + ' -F '+ str(opts['sampling_rate']),shell=True)
            else:
                if opts['separate']:
                    fname = 'sinusoids.cfg'
                    stim_file = '{0},{1}'.format(current_file,modulation_file)
                    sub.call('lcg-rcwrite -e -i -c ' + str(opts['ai'][0]) + ',' + str(opts['ai'][1]) + ' --non-rt -f ' + fname, shell=True)
                    sub.call('lcg-rcwrite -o -c ' + str(opts['ao'][0]) + ',' + str(opts['ao'][1]) + ' --non-rt -f ' + fname + ' -p ' + stim_file, shell=True)
                    sub.call('lcg-non-rt -c ' + fname + ' -F '+ str(opts['sampling_rate']), shell=True)
                else:
                    sub.call('lcg vcclamp -V 3 -f ' + current_file + ' -F '+ str(opts['sampling_rate']), shell=True)

            if cnt != tot:
                sub.call(['sleep', str(opts['interval'])])

if __name__ == '__main__':
    main()

