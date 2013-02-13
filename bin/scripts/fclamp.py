#!/usr/bin/env python

import os
import sys
import numpy as np
import dlutils as dl
import getopt

config_file = 'fclamp.xml'

def usage():
    print('\nUsage: %s [option <value>]' % os.path.basename(sys.argv[0]))
    print('\nwhere options are:\n')
    print('     -f    target frequency.')
    print('     -d    stimulation duration (default 30 s).')
    print('     -a    amplitude of the initially injected current (default 0 pA).')
    print('     -p    proportional gain of the controller (default 0.01).')
    print('     -i    integral gain of the controller (default 0.1).')
    print('     -n    number of repetitions (default 1).')
    print('     -w    interval between repetitions (default 30 s).')
    print('     -I    input channel (default 0).')
    print('     -O    output channel (default 0).')
    print('\n --with-bg add background synaptic noise (default is no).')
    print('\nThe following options are valid only if the --with-bg switch is used:\n')
    print('     -R   input resistance of the cell (in MOhm).')
    print('     -v   value of voltage at which the background activity should be balanced.')
    print('     -F   firing frequency of the excitatory background population.')
    print('')

def main():
    try:
        opts,args = getopt.getopt(sys.argv[1:], 'hf:d:a:n:p:i:w:I:O:R:F:v:', ['help','with-bg'])
    except getopt.GetoptError, err:
        print(str(err))
        usage()
        sys.exit(1)

    ao = 0
    ai = 0
    with_bg = False
    nreps = 1
    gp = 0.01
    gi = 0.1
    duration = 30            # [s]
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
        elif o == '-d':
            duration = float(a)
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

        ratio = dl.computeRatesRatio(balanced_voltage, input_resistance)
        Gm_exc,Gm_inh,Gs_exc,Gs_inh = dl.computeSynapticBackgroundCoefficients(ratio, input_resistance, R_exc)
        dl.writeFClampConfig(target_F, I0, duration, gp, gi, config_file, ai, ao, with_bg, Gm_exc, Gs_exc, Gm_inh, Gs_inh)
    else:
        dl.writeFClampConfig(target_F, I0, duration, gp, gi, config_file, ai, ao)

    os.system('kernel_protocol -I ' + str(ai) + ' -O ' + str(ao))
    os.system('dclamp -c ' + config_file + ' -n ' + str(nreps) + ' -i ' + str(interval))

if __name__ == '__main__':
    main()


