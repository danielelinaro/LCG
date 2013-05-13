#!/usr/bin/env python

import os
import sys
import numpy as np
import lcg
import getopt

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
          
def main():
    try:
        opts,args = getopt.getopt(sys.argv[1:], 'hf:T:t:a:n:p:i:d:t:w:I:O:R:F:v:', ['help','with-bg'])
    except getopt.GetoptError, err:
        print(str(err))
        usage()
        sys.exit(1)

    ao = 0
    ai = 0
    with_bg = False
    nreps = 1
    gp = 0.05
    gi = 1
    gd = 0.
    duration = 30            # [s]
    tau = 1                  # [s]
    interval = 30            # [s]
    I0 = 0                   # [pA]
    target_F = None          # [Hz]
    input_resistance = None  # [MOhm]
    balanced_voltage = None  # [mV]
    R_exc = None             # [Hz]

    for o,a in opts:
        if o in ['-h','--help']:
            usage()
            sys.exit(0)
        elif o == '-f':
            target_F = float(a)
        elif o == '-T':
            duration = float(a)
        elif o == '-t':
            tau = float(a)
        elif o == '-a':
            I0 = float(a)
        elif o == '-n':
            nreps = int(a)
        elif o == '-w':
            interval = float(a)
        elif o == '-I':
            ai = int(a)
        elif o == '-O':
            ao = int(a)
        elif o == '-R':
            input_resistance = float(a)
        elif o == '-F':
            R_exc = float(a)
        elif o == '-v':
            balanced_voltage = float(a)
        elif o == '--with-bg':
            with_bg = True
        elif o == '-p':
            gp = float(a)
        elif o == '-i':
            gi = float(a)
        elif o == '-d':
            gd = float(a)
    
    if not target_F:
        print('You must specify specify the target frequency (-F switch).')
        sys.exit(1)

    if with_bg:
        if not input_resistance:
            print('You must to specify the input resistance of the cell (-R switch).')
            sys.exit(1)
        if not R_exc:
            print('You must specify the firing frequency of the background excitatory population (-F switch).')
            sys.exit(1)
        if not balanced_voltage:
            print('You must to specify the balanced voltage (-v switch).')
            sys.exit(1)

        ratio = lcg.computeRatesRatio(Vm=balanced_voltage, Rin=input_resistance)
        Gm_exc,Gm_inh,Gs_exc,Gs_inh = lcg.computeSynapticBackgroundCoefficients(ratio, R_exc, Rin=input_resistance)
        lcg.writeFClampConfig(target_F, I0, duration, gp, gi, gd, tau, config_file, ai, ao, with_bg, Gm_exc, Gs_exc, Gm_inh, Gs_inh)
    else:
        lcg.writeFClampConfig(target_F, I0, duration, gp, gi, gd, tau, config_file, ai, ao)

    os.system('kernel_protocol -I ' + str(ai) + ' -O ' + str(ao))
    os.system(lcg.common.prog_name + ' -c ' + config_file + ' -n ' + str(nreps) + ' -i ' + str(interval))

if __name__ == '__main__':
    main()


