#!/usr/bin/env python

import sys
import os
import getopt
import numpy as np
import dlutils as dl

gexc_template_file = 'gexc_template.stim'
gexc_file = 'gexc.stim'
ginh_template_file = 'ginh_template.stim'
ginh_file = 'ginh.stim'
current_template_file = 'current_template.stim'
current_file = 'current.stim'
config_file = 'sinusoids.xml'

switches = 'f:n:i:d:I:O:k:R:F:v:a:s:t:m:'
long_switches = ['with-bg']

def usage():
    print('\nUsage: %s <mode> [--option <value>]' % os.path.basename(sys.argv[0]))
    print('\nThe working modes are:\n')
    print('  current      use a sinusoidally modulated current.')
    print('  conductance  use sinusoidally modulated conductances.')
    print('\nThe global options are:\n')
    print('   -h   Display this help message and exit.')
    print('   -f   Frequencies of the sinusoids (comma-separated values).')
    print('   -n   Number of repetitions (default 1).')
    print('   -i   Interval between trials (default 60 s).')
    print('   -d   Duration of the stimulation (default 30 sec).')
    print('   -I   Input channel (default 0).')
    print('   -O   Output channel (default 0).')
    print('   -k   Frequency at which a new kernel should be computed')
    print('        (the default is at the beginning of each batch of frequencies).')

    print('\nThe following options are valid in the "current" working mode:\n')
    print('   -a   Mean of the noisy component of the current (default 0 pA).')
    print('   -s   Standard deviation of the noisy component of the current (default 0 pA).')
    print('   -t   Time constant of the noisy component of the current (default 0 ms).')
    print('   -m   Amplitude of the modulating current (default 30 pA).')

    print('\nAdditionally, if the --with-bg option is specified, the following')
    print('options are accepted:\n')
    print('   -F   Firing frequency of the excitatory background population.')
    print('   -R   Input resistance of the cell (in MOhm).')
    print('   -v   Value of voltage at which the background activity should be balanced.')

    print('\nThe following options are valid in the "conductance" working mode:\n')
    print('   -R   Input resistance of the cell (in MOhm).')
    print('   -v   Value of voltage at which the background activity should be balanced.')
    print('   -F   Baseline firing frequency of the excitatory background population.')
    print('   -m   Fraction of the baseline firing frequency used as a modulation (default 0.1).')
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
               'ai': 0, 'ao': 0}
    options['kernel_frequency'] = len(options['frequencies'])

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
            options['ai'] = int(a)
        elif o == '-O':
            options['ao'] = int(a)
        elif o == '-d':
            options['duration'] = float(a)
        elif o == '-k':
            options['kernel_frequency'] = int(a)

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
        elif o == '-F':
            options['R_exc'] = float(a)
        elif o == '-v':
            options['balanced_voltage'] = float(a)

    if not options['input_resistance']:
        print('You must specify the input resistance of the cell (-R switch).')
        sys.exit(1)
            
    if not options['R_exc']:
        print('You must specify the firing frequency of the background excitatory population (-F switch).')
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
               'I_modul': 30}              # [pA]

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

    for o,a in opts:
        if o == '-m':
            options['dR'] = float(a)

    if options['dR'] < 0:
        print('The modulating fraction must be positive.')
        sys.exit(1)

    return options

def run_frequency(f, mode, opts):
    if mode == 'current':
        os.system('sed -e "s/F/' + str(f) + '/" -e "s/5061983/' + str(np.random.poisson(10000))
                  + '/" ' +  current_template_file + ' > ' + current_file)
        if opts['with_bg']:
            os.system('sed -e "s/5061983/' + str(np.random.poisson(1000)) + '/" ' + gexc_template_file + ' > ' + gexc_file)
            os.system('sed -e "s/5061983/' + str(np.random.poisson(10000)) + '/" ' + ginh_template_file + ' > ' + ginh_file)
            os.system('dclamp -V 3 -c ' + config_file)
        else:
            os.system('cclamp -V 3 -f ' + current_file)
    elif mode == 'conductance':
        os.system('sed -e "s/F/' + str(f) + '/" -e "s/5061983/' + str(np.random.poisson(1000)) + '/" ' + gexc_template_file + ' > ' + gexc_file)
        os.system('sed -e "s/F/' + str(f) + '/" -e "s/7051983/' + str(np.random.poisson(10000)) + '/" ' + ginh_template_file + ' > ' + ginh_file)
        os.system('dclamp -V 3 -c ' + config_file)

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

    if mode == 'current':
        if not opts['with_bg']:
            if opts['mean'] == 0 and opts['std'] == 0 and opts['tau'] == 0:
                # we have only the sinusoidal modulating current
                current = [[opts['duration'],3,opts['I_modul'],'F',0,0,0,0,0,0,0,1],
                           [1,1,0,0,0,0,0,0,0,0,0,1]]
            elif opts['tau'] == 0:
                # sinusoid on top of a DC current
                current = [[opts['duration'],-2,opts['mean'],0,0,0,0,0,0,1,0,1],
                           [0,-2,opts['I_modul'],'F',0,0,0,0,0,3,1,1],
                           [1,1,0,0,0,0,0,0,0,0,0,1]]
            else:
                # sinusoid on top of a noisy (OU) current
                current = [[opts['duration'],-2,opts['mean'],opts['std'],opts['tau'],0,0,1,5061983,2,0,1], # OU current
                           [0,-2,opts['I_modul'],'F',0,0,0,0,0,3,1,1],
                           [1,1,0,0,0,0,0,0,0,0,0,1]]
            dl.writeStimFile(current_template_file, current, True)
        else:
            ratio = dl.computeRatesRatio(opts['balanced_voltage'], opts['input_resistance'])
            Gm_exc,Gm_inh,Gs_exc,Gs_inh = dl.computeSynapticBackgroundCoefficients(ratio, opts['input_resistance'], opts['R_exc'])
            if opts['std'] == 0 and opts['tau'] == 0:
                dl.writeSinusoidsConfig(opts['mean'], opts['I_modul'], Gm_exc, Gs_exc, Gm_inh, Gs_inh,
                                        opts['ai'], opts['ao'], opts['duration'], config_file)
            else:
                dl.writeSinusoidsConfig(opts, opts['I_modul'], Gm_exc, Gs_exc, Gm_inh, Gs_inh,
                                        opts['ai'], opts['ao'], opts['duration'], config_file)
    else:
        ratio = dl.computeRatesRatio(opts['balanced_voltage'], opts['input_resistance'])
        Gm_exc,Gm_inh,Gs_exc,Gs_inh = dl.computeSynapticBackgroundCoefficients(ratio, opts['input_resistance'], opts['R_exc'])
        dl.writeSinusoidsConfig(0, 50, Gm_exc, Gs_exc, Gm_inh, Gs_inh,
                                opts['ai'], opts['ao'], opts['duration'], config_file)
        os.remove(current_template_file)
        os.remove(gexc_template_file)
        os.remove(ginh_template_file)
        dl.writeStimFile(current_file, [[opts['duration']+1,1,0,0,0,0,0,0,0,0,0,1]], True)
        opts['R_inh'] = opts['R_exc']/ratio
        # proportional
        dl.writeSinusoidallyModulatedOU('F', opts['R_exc'], opts['dR']*opts['R_exc'],
                                        opts['input_resistance'], 5, opts['duration'], 'exc',
                                        5061983, gexc_template_file)
        dl.writeSinusoidallyModulatedOU('F', opts['R_inh'], opts['dR']*opts['R_inh'],
                                        opts['input_resistance'], 10, opts['duration'], 'inh',
                                        7051983, ginh_template_file)
        # fixed
        #dl.writeSinusoidallyModulatedOU('F', opts['R_exc'], 550, opts['input_resistance'],
        #                                5, opts['duration'], 'exc', 5061983, gexc_template_file)
        #dl.writeSinusoidallyModulatedOU('F', opts['R_inh'], 550/ratio, opts['input_resistance'],
        #                                10, opts['duration'], 'inh', 7051983, ginh_template_file)

    cnt = 0
    tot = opts['reps']*len(opts['frequencies'])
    for i in range(opts['reps']):
        np.random.shuffle(opts['frequencies'])
        for f in opts['frequencies']:
            if cnt%opts['kernel_frequency'] == 0:
                os.system('kernel_protocol -I ' + str(opts['ai']) + ' -O ' + str(opts['ao']))
            cnt = cnt+1
            print('[%02d/%02d] F = %g Hz.' % (cnt,tot,f))
            run_frequency(f, mode, opts)
            if cnt != tot:
                os.system('sleep ' + str(opts['interval']))

if __name__ == '__main__':
    main()

