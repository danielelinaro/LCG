#!/usr/bin/env python

import sys
import os
import getopt
import numpy as np
import lcg
import time

stim_files = {'gexc': 'gexc.stim', 'gexc_common': 'gexc_common.stim',
              'ginh': 'ginh.stim', 'ginh_common': 'ginh_common.stim'}

try:
    config_template_file = os.environ['CONFIGURATIONS_PATH'] + '/correlations_template.xml'
except:
    config_template_file = os.environ['HOME'] + '/configurations/correlations_template.xml'
config_file = 'correlations.xml'

def usage():
    print('\nUsage: %s [--option <value>]' % os.path.basename(sys.argv[0]))
    print('\nThe options are:\n')
    print('     -h   Display this help message and exit.')
    print('     -v   Values of voltage at which the background activity should be balanced (comma-separated values).')
    print('     -c   Correlation coefficients (comma-separated values).')
    print('     -F   Firing frequency of the excitatory background population.')
    print('     -R   Input resistance of the cell (in MOhm).')
    print('     -n   Number of repetitions (default 100).')
    print('     -i   Interval between trials (default 2 s).')
    print('     -d   Duration of the stimulation (default 1.1 sec).')
    print('     -I   Input channel (default 0).')
    print('     -O   Output channel (default 0).')
    print('     -k   Frequency at which a new kernel should be computed')
    print('          (the default is just at the beginning).')
    print('     -b   Time before stimulus onset (default 0.1 sec).')
    print('     -a   Time after stimulus offset (default 0.3 sec).')
    print('')

def parseArgs():
    try:
        opts,args = getopt.getopt(sys.argv[1:],'hc:n:i:d:I:O:k:R:F:v:b:a:',['help'])
    except getopt.GetoptError, err:
        print(str(err))
        usage()
        sys.exit(1)

    options = {'reps': 100,
               'interval': 2,   # [s]
               'duration': 1.1,   # [s]
               'kernel_frequency': 0,
               'correlation_coeffs': None,
               'balanced_voltages': None,
               'input_resistance': None,
               'R_exc': None,
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
        elif o == '-R':
            options['input_resistance'] = float(a)
        elif o == '-F':
            options['R_exc'] = float(a)
        elif o == '-v':
            options['balanced_voltages'] = []
            for v in a.split(','):
                options['balanced_voltages'].append(float(v))
        elif o == '-c':
            options['correlation_coefficients'] = []
            for c in a.split(','):
                options['correlation_coefficients'].append(float(c))

    if not options['input_resistance']:
        print('You must specify the input resistance of the cell (-R switch).')
        sys.exit(1)
            
    if not options['R_exc']:
        print('You must specify the firing frequency of the background excitatory population (-F switch).')
        sys.exit(1)
                
    if not options['balanced_voltages']:
        print('You must specify the balanced voltage (-v switch)')
        sys.exit(1)

    if not options['correlation_coefficients']:
        print('You must specify the balanced voltage (-c switch)')
        sys.exit(1)

    if options['kernel_frequency'] <= 0:
        options['kernel_frequency'] = options['reps']

    return options

def main():
    opts = parseArgs()
    lcg.substituteStrings(config_template_file, config_file,
                         {'>AI<': '>'+str(opts['ai'])+'<', '>AO<': '>'+str(opts['ao'])+'<',
                          'AI_CONVERSION_FACTOR': os.environ['AI_CONVERSION_FACTOR'],
                          'AO_CONVERSION_FACTOR': os.environ['AO_CONVERSION_FACTOR'],
                          'TEND': 5})

    cnt = 0
    tot = len(opts['balanced_voltages']) * len(opts['correlation_coefficients']) * opts['reps']
    gexc = {'m': 0, 's': 0, 'mc': 0, 'sc': 0}
    ginh = {'m': 0, 's': 0, 'mc': 0, 'sc': 0}

    stim = [[opts['before'], 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1],
            [opts['duration'], 2, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1],
            [opts['after'], 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1]]

    np.random.seed(5061983)
    exc_seeds = map(int, np.random.uniform(low=0, high=10000, size=opts['reps']))
    np.random.seed(7051983)
    inh_seeds = map(int, np.random.uniform(low=0, high=10000, size=opts['reps']))

    np.random.seed(int(time.time()))

    np.random.shuffle(opts['balanced_voltages'])
    np.random.shuffle(opts['correlation_coefficients'])

    for v in opts['balanced_voltages']:
        ratio = lcg.computeRatesRatio(Vm=v, Rin=opts['input_resistance'])
        for c in opts['correlation_coefficients']:
            gexc['m'],ginh['m'],gexc['s'],ginh['s'] = lcg.computeSynapticBackgroundCoefficients(ratio,
                                                                                                R_exc=(1-c)*opts['R_exc'],
                                                                                                Rin=opts['input_resistance'])
            gexc['mc'],ginh['mc'],gexc['sc'],ginh['sc'] = lcg.computeSynapticBackgroundCoefficients(ratio,
                                                                                                    R_exc=c*opts['R_exc'],
                                                                                                    Rin=opts['input_resistance'])

            for k in range(opts['reps']):
                if cnt%opts['kernel_frequency'] == 0:
                    sub.call('kernel_protocol -I ' + str(opts['ai']) + ' -O ' + str(opts['ao']), shell=True)
                cnt = cnt+1

                stim[1][2] = gexc['m']
                stim[1][3] = gexc['s']
                stim[1][4] = 5
                stim[1][8] = int(np.random.uniform(low=0, high=100*opts['reps']))
                lcg.writeStimFile(stim_files['gexc'], stim, False)
                
                stim[1][2] = ginh['m']
                stim[1][3] = ginh['s']
                stim[1][4] = 10
                stim[1][8] = int(np.random.uniform(low=0, high=100*opts['reps']))
                lcg.writeStimFile(stim_files['ginh'], stim, False)
                
                stim[1][2] = gexc['mc']
                stim[1][3] = gexc['sc']
                stim[1][4] = 5
                stim[1][8] = exc_seeds[k]
                lcg.writeStimFile(stim_files['gexc_common'], stim, False)
                
                stim[1][2] = ginh['mc']
                stim[1][3] = ginh['sc']
                stim[1][4] = 10
                stim[1][8] = inh_seeds[k]
                lcg.writeStimFile(stim_files['ginh_common'], stim, False)

                sub.call('dclamp -V 4 -c ' + config_file, shell=True)
                sub.call(['sleep', str(opts['interval'])])
                
                if cnt%10 == 0:
                    print('[%02d/%02d]' % (cnt,tot))


if __name__ == '__main__':
    main()

