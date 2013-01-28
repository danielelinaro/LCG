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
current_template_file = 'template.stim'
current_file = 'current.stim'
config_file = 'sinusoids.xml'

def usage():
    print('\nUsage: %s [option <value>]' % os.path.basename(sys.argv[0]))
    print('\nwhere options are:\n')
    print('   -h   display this help message and exit')
    print('   -f   the frequencies of the sinusoids (comma-separate values).')
    print('   -n   the number of repetitions (default 1)')
    print('   -i   the interval between trials (default 60 s)')
    print('   -d   the duration of the stimulation (default 30 sec).')
    print('   -a   the DC component of the current (default 0 pA)')
    print('   -m   the amplitude of the modulating current (default 30 pA)')
    print('   -I   input channel (default 0)')
    print('   -O   output channel (default 0)')
    print('   -k   the frequency at which a new kernel should be computed')
    print('         (the default is at the beginning of each batch of frequencies)')
    print('\n  --without-bg   turn off the simulated background activity')
    print('\nThe following options are taken into account only if the switch --without-bg is not present:\n')
    print('   -R   input resistance of the cell (in MOhm).')
    print('   -F   firing frequency of the excitatory background population.')
    print('   -v   value of voltage at which the background activity should be balanced.')
    print('')

def run_frequency(f, with_bg=True):
    os.system('sed -e "s/F/' + str(f) + '/" ' + current_template_file + ' > ' + current_file)
    if with_bg:
        os.system('sed -e "s/5061983/' + str(np.random.poisson(1000)) + '/" ' + gexc_template_file + ' > ' + gexc_file)
        os.system('sed -e "s/5061983/' + str(np.random.poisson(10000)) + '/" ' + ginh_template_file + ' > ' + ginh_file)
        os.system('dclamp -V 3 -c ' + config_file)
    else:
        pass
        os.system('cclamp -V 3 -f ' + current_file)

def main():
    try:
        opts,args = getopt.getopt(sys.argv[1:],'hf:n:i:R:F:v:d:a:m:k:I:O:', ['help', 'without-bg'])
    except getopt.GetoptError, err:
        print str(err)
        usage()
        sys.exit(1)

    frequencies = []  # [Hz]
    reps = 1          # [1]
    interval = 60     # [s]
    ai = 0
    ao = 0
    input_resistance = None # [MOhm]
    bg_freq = None          # [Hz]
    balanced_voltage = None # [mV]
    I0 = 0           # [pA]
    I1 = 30          # [pA]
    duration = 30    # [s]
    with_bg = True
    kernel_frequency = None

    for o,a in opts:
        if o in ('-h','--help'):
            usage()
            sys.exit(0)
        elif o == '-f':
            for f in a.split(','):
                frequencies.append(float(f))
        elif o == '-n':
            reps = int(a)
        elif o == '-i':
            interval = float(a)
        elif o == '-I':
            ai = int(a)
        elif o == '-O':
            ao = int(a)
        elif o == '-R':
            input_resistance = float(a)
        elif o == '-F':
            bg_freq = float(a)
        elif o == '-v':
            balanced_voltage = float(a)
        elif o == '-d':
            duration = float(a)
        elif o == '-a':
            I0 = float(a)
        elif o == '-m':
            I1 = float(a)
        elif o == '--without-bg':
            with_bg = False
        elif o == '-k':
            kernel_frequency = int(a)

    if len(frequencies) == 0:
        frequencies = [1,5,10,20,50,100,200,300,600,1000]

    if not kernel_frequency:
        kernel_frequency = len(frequencies)

    if with_bg:
        if not input_resistance:
            print('You must specify the input resistance of the cell (-R switch).')
            sys.exit(1)
            
        if not bg_freq:
            print('You must specify the firing frequency of the background excitatory population (-F switch).')
            sys.exit(1)
                
        if not balanced_voltage:
            print('You must specify the balanced voltage (-v switch)')
            sys.exit(1)

        ratio = dl.computeRatesRatio(balanced_voltage, input_resistance)
        Gm_exc,Gm_inh,Gs_exc,Gs_inh = dl.computeSynapticBackgroundCoefficients(ratio, input_resistance, bg_freq)
        dl.writeSinusoidsConfig(I0, I1, Gm_exc, Gs_exc, Gm_inh, Gs_inh, ai, ao, duration, config_file)
    else:
        current = [[duration,-2,I0,0,0,0,0,0,0,1,0,1],
                   [0,-2,I1,'F',0,0,0,0,0,3,1,1],
                   [1,1,0,0,0,0,0,0,0,0,0,1]]
        dl.writeStimFile(current_template_file, current, True)

    cnt = 0
    tot = reps*len(frequencies)
    for i in range(reps):
        np.random.shuffle(frequencies)
        for f in frequencies:
            if cnt%kernel_frequency == 0:
                os.system('kernel_protocol -I ' + str(ai) + ' -O ' + str(ao))
            cnt = cnt+1
            print('[%02d/%02d] F = %g Hz.' % (cnt,tot,f))
            run_frequency(f, with_bg)
            if cnt != tot:
                os.system('sleep ' + str(interval))

if __name__ == '__main__':
    main()

