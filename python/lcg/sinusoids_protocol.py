#!/usr/bin/env python

import sys
import os
import getopt
import subprocess as sub
import numpy as np
import lcg

gexc_template_file = 'gexc_template.stim'
gexc_file = 'gexc.stim'
ginh_template_file = 'ginh_template.stim'
ginh_file = 'ginh.stim'
current_template_file = 'current_template.stim'
current_file = 'current.stim'
config_file = 'sinusoids.xml'

switches = 'f:n:i:d:I:O:k:R:r:F:v:a:s:t:m:'
long_switches = ['with-bg','exc','inh','no-kernel']

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
               'srate' : 20000,  # [Hz]
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
            options['ai'] = int(a)
        elif o == '-O':
            options['ao'] = int(a)
        elif o == '-F':
            options['srate'] = float(a)
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

def run_frequency(f, mode, opts):
    if mode == 'current':
        sub.call('sed -e "s/F/' + str(f) + '/" -e "s/5061983/' + str(np.random.poisson(10000), shell=True)
                  + '/" ' +  current_template_file + ' > ' + current_file, shell=True)
        if opts['with_bg']:
            sub.call('sed -e "s/5061983/' + str(np.random.poisson(1000)) + '/" ' + gexc_template_file + ' > ' + gexc_file, shell=True)
            sub.call('sed -e "s/5061983/' + str(np.random.poisson(10000)) + '/" ' + ginh_template_file + ' > ' + ginh_file, shell=True)
            sub.call('dclamp -V 3 -c ' + config_file + ' -F '+ str(opts['srate']), shell=True)
        else:
            sub.call('cclamp -V 3 -f ' + current_file + ' -F '+ str(opts['srate']),shell=True)
    elif mode == 'conductance':
        sub.call('sed -e "s/F/' + str(f) + '/" -e "s/5061983/' + str(np.random.poisson(1000)) + '/" ' + gexc_template_file + ' > ' + gexc_file, shell=True)
        sub.call('sed -e "s/F/' + str(f) + '/" -e "s/7051983/' + str(np.random.poisson(10000)) + '/" ' + ginh_template_file + ' > ' + ginh_file, shell=True)
        sub.call('dclamp -V 3 -c ' + config_file + ' -F '+ str(opts['srate']),shell=True)

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
            lcg.writeStimFile(current_template_file, current, True)
        else:
            ratio = lcg.computeRatesRatio(Vm=opts['balanced_voltage'], Rin=opts['input_resistance'])
            Gm_exc,Gm_inh,Gs_exc,Gs_inh = lcg.computeSynapticBackgroundCoefficients(ratio, R_exc=opts['R_exc'], Rin=opts['input_resistance'])
            if opts['std'] == 0 and opts['tau'] == 0:
                lcg.writeSinusoidsConfig(opts['mean'], opts['I_modul'], Gm_exc, Gs_exc, Gm_inh, Gs_inh,
                                        opts['ai'], opts['ao'], opts['duration'], config_file)
            else:
                lcg.writeSinusoidsConfig(opts, opts['I_modul'], Gm_exc, Gs_exc, Gm_inh, Gs_inh,
                                        opts['ai'], opts['ao'], opts['duration'], config_file)
    else:
        ratio = lcg.computeRatesRatio(Vm=opts['balanced_voltage'], Rin=opts['input_resistance'])
        Gm_exc,Gm_inh,Gs_exc,Gs_inh = lcg.computeSynapticBackgroundCoefficients(ratio, R_exc=opts['R_exc'], Rin=opts['input_resistance'])
        lcg.writeSinusoidsConfig(0, 50, Gm_exc, Gs_exc, Gm_inh, Gs_inh,
                                opts['ai'], opts['ao'], opts['duration'], config_file)
        os.remove(current_template_file)
        lcg.writeStimFile(current_file, [[opts['duration']+1,1,0,0,0,0,0,0,0,0,0,1]], True)
        opts['R_inh'] = opts['R_exc']/ratio

        # proportional
        if opts['exc']:
            os.remove(gexc_template_file)
            lcg.writeSinusoidallyModulatedOU('F', opts['R_exc'], opts['dR']*opts['R_exc'],
                                             opts['input_resistance'], 5, opts['duration'], 'exc',
                                             5061983, gexc_template_file)

        if opts['inh']:
            os.remove(ginh_template_file)
            lcg.writeSinusoidallyModulatedOU('F', opts['R_inh'], opts['dR']*opts['R_inh'],
                                             opts['input_resistance'], 10, opts['duration'], 'inh',
                                             7051983, ginh_template_file)

        # fixed
        #if opts['exc']:
        #    os.remove(gexc_template_file)
        #    lcg.writeSinusoidallyModulatedOU('F', opts['R_exc'], 550, opts['input_resistance'],
        #                                     5, opts['duration'], 'exc', 5061983, gexc_template_file)
        #if opts['inh']:
        #    os.remove(ginh_template_file)
        #    lcg.writeSinusoidallyModulatedOU('F', opts['R_inh'], 550/ratio, opts['input_resistance'],
        #                                     10, opts['duration'], 'inh', 7051983, ginh_template_file)

    cnt = 0
    tot = opts['reps']*len(opts['frequencies'])
    for i in range(opts['reps']):
        np.random.shuffle(opts['frequencies'])
        for f in opts['frequencies']:
            if opts['compute_kernel'] and cnt%opts['kernel_frequency'] == 0:
                sub.call('kernel_protocol -I ' + str(opts['ai']) + ' -O ' + str(opts['ao']) + ' -F '+ str(opts['srate']), shell=True)
            cnt = cnt+1
            print('[%02d/%02d] F = %g Hz.' % (cnt,tot,f))
            run_frequency(f, mode, opts)
            if cnt != tot:
                sub.call(['sleep', str(opts['interval'])])

if __name__ == '__main__':
    main()

