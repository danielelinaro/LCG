#!/usr/bin/env python

import sys
import os
import getopt
import numpy as np
import lcg
import time
import subprocess as sub

class ColorFactory:
    RED = '\033[91m'
    GREEN = '\033[92m'
    BLUE = '\033[94m'
    YELLOW = '\033[93m'
    ENDC = '\033[0m'
    def __init__(self):
        pass
    def __color__(self, col, str):
        return col + str + self.ENDC
    def red(self, str):
        return self.__color__(self.RED, str)
    def green(self, str):
        return self.__color__(self.GREEN, str)
    def blue(self, str):
        return self.__color__(self.BLUE, str)
    def yellow(self, str):
        return self.__color__(self.YELLOW, str)

stim_files = {'current': 'current.stim',
              'gampa': 'gampa.stim', 'gampa_common': 'gampa_common.stim',
              'ggaba': 'ggaba.stim', 'ggaba_common': 'ggaba_common.stim',
              'gnmda': 'gnmda.stim', 'gnmda_common': 'gnmda_common.stim'}
config_file = 'correlations.xml'
colors = ColorFactory()
switches = 'hc:n:i:d:I:O:k:R:f:F:v:b:a:m:s:t:K:'
long_switches = ['help','with-nmda']

def usage():
    print('\nUsage: %s <mode> [--option <value>]' % os.path.basename(sys.argv[0]))
    print('\nThe working modes are:\n')
    print('    ' + colors.yellow('current') + '      Perform a current-clamp experiment.')
    print('    ' + colors.green('conductance') + '  Perform a current-clamp experiment.')
    print('\nThe global options are:\n')
    print('     -h   Display this help message and exit.')
    print('     -c   Correlation coefficients (comma-separated values).')
    print('     -n   Number of repetitions (default 100).')
    print('     -i   Interval between trials (default 2 s).')
    print('     -d   Duration of the stimulation (default 1.1 sec).')
    print('     -I   Input channel (default 0).')
    print('     -O   Output channel (default 0).')
    print('     -F   Sampling rate (default %s Hz).' % os.environ['SAMPLING_RATE'])
    print('     -k   Frequency at which a new kernel should be computed (the default is just at the beginning).')
    print('     -b   Time before stimulus onset (default 0.1 sec).')
    print('     -a   Time after stimulus offset (default 0.3 sec).')
    print('\nThe following options are valid in the "' + colors.yellow('current') + '" working mode:\n')
    print('     ' + colors.yellow('-m') + '   Mean of the noisy current.')
    print('     ' + colors.yellow('-s') + '   Standard deviation of the noisy current.')
    print('     ' + colors.yellow('-t') + '   Time constant of the noisy current (default -1 ms, which means Gaussian noise).')
    print('\nThe following options are valid in the "' + colors.green('conductance') + '" working mode:\n')
    print('     ' + colors.green('-v') + '   Values of voltage at which the background activity should be balanced (comma-separated values).')
    print('     ' + colors.green('-f') + '   Firing frequency of the inhibitory background population (default 3000 Hz).')
    print('     ' + colors.green('-R') + '   Input resistance of the cell (in MOhm).')
    print('\nAdditionally, if the ' + colors.blue('--with-nmda') + ' option is set, the following options are accepted:\n')
    print('     ' + colors.blue('-m') + '   Mean of the NMDA conductance (default 0 nS).')
    print('     ' + colors.blue('-s') + '   Standard deviation of the NMDA conductance (default twice the AMPA conductance).')
    print('     ' + colors.blue('-t') + '   Time constant of the NMDA conductance (default 100 ms).')
    print('     ' + colors.blue('-K') + '   Coefficients for the magnesium block of the NMDA conductance (default 0.2,0.02).')
    print('')

def parseGlobalArgs():
    try:
        opts,args = getopt.getopt(sys.argv[2:],switches,long_switches)
    except getopt.GetoptError, err:
        print(str(err))
        usage()
        sys.exit(1)

    options = {'reps': 100,
               'interval': 2,   # [s]
               'duration': 1.1,   # [s]
               'kernel_frequency': 0,
               'correlation_coefficients': None,
               'sampling_rate': float(os.environ['SAMPLING_RATE']), # [Hz]
               'before': 0.1, 'after': 0.3, # [s]
               'ai': 0, 'ao': 0}

    for o,a in opts:
        if o in ('-h','--help'):
            usage()
            sys.exit(1)
        elif o == '-n':
            options['reps'] = int(a)
        elif o == '-i':
            options['interval'] = float(a)
        elif o == '-b':
            options['before'] = float(a)
        elif o == '-a':
            options['after'] = float(a)
        elif o == '-I':
            options['ai'] = int(a)
        elif o == '-O':
            options['ao'] = int(a)
        elif o == '-d':
            options['duration'] = float(a)
        elif o == '-k':
            options['kernel_frequency'] = int(a)
        elif o == '-F':
            options['sampling_rate'] = float(a)
        elif o == '-c':
            options['correlation_coefficients'] = []
            for c in a.split(','):
                options['correlation_coefficients'].append(float(c))
        
    if options['sampling_rate'] <= 0:
        print('The sampling rate must be greater than 0.')
        sys.exit(1)

    if not options['correlation_coefficients']:
        print('You must specify the correlation coefficient(s) (-c switch).')
        sys.exit(1)

    if min(options['correlation_coefficients']) < 0 or max(options['correlation_coefficients']) > 1:
        plural = ''
        if len(options['correlation_coefficients']) > 1:
            plural = 's'
        print('The correlation coefficient' + plural + ' must be between 0 and 1.')
        sys.exit(1)

    if options['kernel_frequency'] <= 0:
        options['kernel_frequency'] = options['reps']

    return options

def parseCurrentModeArgs():
    try:
        opts,args = getopt.getopt(sys.argv[2:],switches,long_switches)
    except getopt.GetoptError, err:
        print(str(err))
        usage()
        sys.exit(1)

    options = {'current_mean': None, # [pA]
               'current_std': None,  # [pA]
               'current_tau': -1}  # [ms]

    for o,a in opts:
        if o == '-m':
            options['current_mean'] = float(a)
        elif o == '-s':
            options['current_std'] = float(a)
        elif o == '-t':
            options['current_tau'] = float(a)

    if not options['current_mean']:
        print('You must specify the mean of the current (-m switch).')
        sys.exit(1)

    if not options['current_std']:
        print('You must specify the standard deviation of the current (-s switch).')
        sys.exit(1)

    if options['current_std'] < 0:
        print('The standard deviation of the current must be non-negative.')
        sys.exit(1)

    return options

def parseConductanceModeArgs():
    try:
        opts,args = getopt.getopt(sys.argv[2:],switches,long_switches)
    except getopt.GetoptError, err:
        print(str(err))
        usage()
        sys.exit(1)

    options = {'balanced_voltages': None,
               'input_resistance': None,
               'R_exc': 7000, # [Hz]
               'R_inh': 3000, # [Hz]
               'with_nmda': False,
               'nmda_mean': 1, 'nmda_std': 1,   ### dummy values, to remove
               'nmda_tau': 100, 'nmda_K': [0.2,0.02]}

    for o,a in opts:
        if o == '-R':
            options['input_resistance'] = float(a)
        elif o == '-f':
            options['R_inh'] = float(a)
        elif o == '-v':
            options['balanced_voltages'] = []
            for v in a.split(','):
                options['balanced_voltages'].append(float(v))
        elif o == '--with-nmda':
            options['with_nmda'] = True
        elif o == '-m':
            options['nmda_mean'] = float(a)
        elif o == '-s':
            options['nmda_std'] = float(a)
        elif o == '-t':
            options['nmda_tau'] = float(a)
        elif o == '-K':
            for i,k in enumerate(a.split(',')):
                options['nmda_K'][i] = float(k)

    if not options['input_resistance']:
        print('You must specify the input resistance of the cell (-R switch).')
        sys.exit(1)
    
    if options['input_resistance'] <= 0:
        print('The input resistance must be positive.')
        sys.exit(1)

    if options['R_inh'] <= 0:
        print('The firing frequency of the inhibitory population must be positive.')
        sys.exit(1)
        
    if not options['balanced_voltages']:
        print('You must specify the balanced voltage (-v switch)')
        sys.exit(1)

    if min(options['balanced_voltages']) < -80 or max(options['balanced_voltages']) > 0:
        plural = ''
        if len(options['balanced_voltages']) > 1:
            plural = 's'
        print('The balanced voltage' + plural + ' must be between -80 and 0 mV.')
        sys.exit(1)

    if options['with_nmda']:
        if options['nmda_mean'] < 0:
            print('You must specify a non-negative mean for the NMDA conductance.')
            sys.exit(1)
        if options['nmda_std'] != None and options['nmda_std'] < 0:
            print('You must specify a non-negative standard deviation for the NMDA conductance.')
            sys.exit(1)
        if options['nmda_tau'] <= 0:
            print('The time constant of the NMDA conductance must be positive')
            sys.exit(1)
        if not np.all(np.array(options['nmda_K']) >= 0):
            print('The K1 and K2 coefficients must be non-negative.')
            sys.exit(1)
        if options['nmda_mean'] == 0 and options['nmda_std'] == 0:
            print('Both mean and standard deviation of the NMDA conductance are zero, I cowardly refuse to continue.')
            sys.exit(0)

    return options

def writeConfigFile(options):
    config = lcg.XMLConfigurationFile(options['sampling_rate'],options['duration']+options['before']+options['after'])
    config.add_entity(lcg.entities.H5Recorder(id=0, connections=(), compress=True))
    config.add_entity(lcg.entities.RealNeuron(id=1, connections=(0), spikeThreshold=-20, V0=-65, deviceFile=os.environ['COMEDI_DEVICE'],
                                              inputSubdevice=os.environ['AI_SUBDEVICE'],
                                              outputSubdevice=os.environ['AO_SUBDEVICE'],
                                              readChannel=options['ai'], writeChannel=options['ao'],
                                              inputConversionFactor=os.environ['AI_CONVERSION_FACTOR_CC'],
                                              outputConversionFactor=os.environ['AO_CONVERSION_FACTOR_CC'],
                                              inputRange=os.environ['RANGE'], reference=os.environ['GROUND_REFERENCE'],
                                              kernelFile='kernel.dat'))
    config.add_entity(lcg.entities.Waveform(id=2, connections=(0,3), filename=stim_files['gampa'], units='nS'))
    config.add_entity(lcg.entities.ConductanceStimulus(id=3, connections=(1), E=0.))
    config.add_entity(lcg.entities.Waveform(id=4, connections=(0,5), filename=stim_files['ggaba'], units='nS'))
    config.add_entity(lcg.entities.ConductanceStimulus(id=5, connections=(1), E=-80.))
    config.add_entity(lcg.entities.Waveform(id=6, connections=(0,7), filename=stim_files['gampa_common'], units='nS'))
    config.add_entity(lcg.entities.ConductanceStimulus(id=7, connections=(1), E=0.))
    config.add_entity(lcg.entities.Waveform(id=8, connections=(0,9), filename=stim_files['ggaba_common'], units='nS'))
    config.add_entity(lcg.entities.ConductanceStimulus(id=9, connections=(1), E=-80.))
    if options['with_nmda']:
        config.add_entity(lcg.entities.Waveform(id=10, connections=(0,11), filename=stim_files['gnmda'], units='nS'))
        config.add_entity(lcg.entities.NMDAConductanceStimulus(id=11, connections=(1),
                                                               E=0., K1=options['nmda_K'][0], K2=options['nmda_K'][1]))
        config.add_entity(lcg.entities.Waveform(id=12, connections=(0,13), filename=stim_files['gnmda_common'], units='nS'))
        config.add_entity(lcg.entities.NMDAConductanceStimulus(id=13, connections=(1),
                                                               E=0., K1=options['nmda_K'][0], K2=options['nmda_K'][1]))
    config.write(config_file)

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
        tot = len(opts['correlation_coefficients']) * opts['reps']
        stim = [[opts['before'], 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1],
                [opts['duration'], -2, opts['current_mean'], 0, opts['current_tau'], 0, 0, 1, 0, 2, 0, 1], # indipendent part
                [0, -2, 0, 0, opts['current_tau'], 0, 0, 1, 0, 2, 1, 1], # common part
                [opts['after'], 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1]]
        I = {'m': 0, 's': 0, 'mc': 0, 'sc': 0}
        np.random.seed(5061983)
        I_seeds = map(int, np.random.uniform(low=0, high=10000, size=opts['reps']))
    else:
        opts = dict(parseConductanceModeArgs(), **opts)
        writeConfigFile(opts)
        tot = len(opts['balanced_voltages']) * len(opts['correlation_coefficients']) * opts['reps']
        stim = [[opts['before'], 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1],
                [opts['duration'], 2, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1],
                [opts['after'], 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1]]
        gampa = {'m': 0, 's': 0, 'mc': 0, 'sc': 0}
        ggaba = {'m': 0, 's': 0, 'mc': 0, 'sc': 0}
        np.random.seed(5061983)
        ampa_seeds = map(int, np.random.uniform(low=0, high=10000, size=opts['reps']))
        np.random.seed(7051983)
        gaba_seeds = map(int, np.random.uniform(low=0, high=10000, size=opts['reps']))
        if opts['with_nmda']:
            np.random.seed(723587)
            gnmda = {'m': 0, 's': 0, 'mc': 0, 'sc': 0}
            nmda_seeds = map(int, np.random.uniform(low=0, high=10000, size=opts['reps']))

    np.random.seed(int(time.time()))
    np.random.shuffle(opts['correlation_coefficients'])

    cnt = 0
    for c in opts['correlation_coefficients']:
        if mode == 'current':
            stim[1][3] = np.sqrt(1-c)*opts['current_std']
            stim[2][3] = np.sqrt(c)*opts['current_std']
            for k in range(opts['reps']):
                if cnt%opts['kernel_frequency'] == 0:
                    sub.call('lcg kernel -I ' + str(opts['ai']) + ' -O ' + str(opts['ao']), shell=True)
                stim[1][8] = int(np.random.uniform(low=0, high=100*opts['reps']))
                stim[2][8] = I_seeds[k]
                lcg.writeStimFile(stim_files['current'], stim, False)
                sub.call('lcg vcclamp -V 4 -f ' + stim_files['current'], shell=True)
                sub.call(['sleep', str(opts['interval'])])
                cnt = cnt+1
                if cnt%10 == 0:
                    print('[%02d/%02d]' % (cnt,tot))
        else:
            np.random.shuffle(opts['balanced_voltages'])
            if opts['with_nmda']:
                gnmda['m'] = (1-c)*opts['nmda_mean']
                gnmda['mc'] = c*opts['nmda_mean']
                gnmda['s'] = np.sqrt(1-c)*opts['nmda_std']
                gnmda['sc'] = np.sqrt(c)*opts['nmda_std']
            for v in opts['balanced_voltages']:
                ratio = lcg.computeRatesRatio(Vm=v, Rin=opts['input_resistance'])
                gampa['m'],ggaba['m'],gampa['s'],ggaba['s'] = lcg.computeSynapticBackgroundCoefficients(
                    ratio,R_exc=ratio*(1-c)*opts['R_inh'],Rin=opts['input_resistance'])
                gampa['mc'],ggaba['mc'],gampa['sc'],ggaba['sc'] = lcg.computeSynapticBackgroundCoefficients(
                    ratio,R_exc=ratio*c*opts['R_inh'],Rin=opts['input_resistance'])
                for k in range(opts['reps']):
                    stim[1][2] = gampa['m']
                    stim[1][3] = gampa['s']
                    stim[1][4] = 5
                    stim[1][8] = int(np.random.uniform(low=0, high=100*opts['reps']))
                    lcg.writeStimFile(stim_files['gampa'], stim, False)
                    stim[1][2] = ggaba['m']
                    stim[1][3] = ggaba['s']
                    stim[1][4] = 10
                    stim[1][8] = int(np.random.uniform(low=0, high=100*opts['reps']))
                    lcg.writeStimFile(stim_files['ggaba'], stim, False)
                    stim[1][2] = gampa['mc']
                    stim[1][3] = gampa['sc']
                    stim[1][4] = 5
                    stim[1][8] = ampa_seeds[k]
                    lcg.writeStimFile(stim_files['gampa_common'], stim, False)
                    stim[1][2] = ggaba['mc']
                    stim[1][3] = ggaba['sc']
                    stim[1][4] = 10
                    stim[1][8] = gaba_seeds[k]
                    lcg.writeStimFile(stim_files['ggaba_common'], stim, False)
                    if opts['with_nmda']:
                        stim[1][2] = gnmda['m']
                        stim[1][3] = gnmda['s']
                        stim[1][4] = opts['nmda_tau']
                        stim[1][8] = int(np.random.uniform(low=0, high=100*opts['reps']))
                        lcg.writeStimFile(stim_files['gnmda'], stim, False)
                        stim[1][2] = gnmda['m']
                        stim[1][3] = gnmda['s']
                        stim[1][4] = opts['nmda_tau']
                        stim[1][8] = nmda_seeds[k]
                        lcg.writeStimFile(stim_files['gnmda_common'], stim, False)
                    if cnt%opts['kernel_frequency'] == 0:
                        sub.call('lcg kernel -I ' + str(opts['ai']) + ' -O ' + str(opts['ao']), shell=True)
                    cnt = cnt+1
                    sub.call(lcg.common.prog_name + ' -V 4 -c ' + config_file, shell=True)
                    sub.call(['sleep', str(opts['interval'])])
                    if cnt%10 == 0:
                        print('[%02d/%02d]' % (cnt,tot))

if __name__ == '__main__':
    main()

