#!/usr/bin/env python

import os
import sys
import getopt
import glob
import numpy as np
import lcg

def frequency_error(Vbal, target, Rm, R_exc, ai=0, ao=0, duration=10, interval=1, dclamp='dclamp'):
    ratio = lcg.computeRatesRatio(Vbal, Rm)
    G0_exc,G0_inh,sigma_exc,sigma_inh = lcg.computeSynapticBackgroundCoefficients(ratio[0], Rm, R_exc)
    lcg.writeSpontaneousConfig(0, G0_exc, sigma_exc, G0_inh, sigma_inh, ai, ao, duration, outfile='spontaneous.xml')
    if interval > 0:
        os.system('sleep ' + str(interval))
    os.system(dclamp + ' -c spontaneous.xml -V 4')     # run dclamp
    files = glob.glob('*.h5')
    files.sort()
    files = files[-1]
    entities,info = lcg.loadH5Trace(files)
    for ntt in entities:
        if ntt['name'] == 'RealNeuron':
            V = ntt['data']
            break
    if max(V) < -40:    # no spike in the presynaptic
        print('Balanced voltage: %.2f mV.' % Vbal)
        print('Spontaneous firing frequency: 0 Hz (0 spikes).')
        print('Error = %g Hz^2.' % target**2)
        return target**2
    t = np.arange(0, len(V)) * info['dt']
    spks = lcg.findSpikes(t,V,-20)
    freq = float(len(spks)) / duration
    print('Balanced voltage: %.2f mV.' % Vbal)
    print('Spontaneous firing frequency: %.3f Hz (%d spikes).' % (freq, len(spks)))
    print('Error = %g Hz^2.' % (freq-target)**2)
    return (freq - target)**2

def usage():
    print('\nUsage: %s [option <value>]' % os.path.basename(sys.argv[0]))
    print('\nwhere options are:\n')
    print('     -h    display this help message and exit.')
    print('     -R    input resistance of the cell in MOhm.')
    print('     -f    target frequency (default 1 Hz).')
    print('     -r    background synaptic rate (default 7000 Hz).')
    print('     -i    interval between repetitions (default 1 s).')
    print('     -d    duration of the stimulation (default 10 s).')
    print('     -V    minimum (hyperpolarized) voltage (default -60 mV).')
    print('     -v    maximum (depolarized) voltage (default -40 mV).')
    print('     -I    input channel (default 0).')
    print('     -O    output channel (default 0).\n')
    print('')

def main():
    try:
        opts,args = getopt.getopt(sys.argv[1:], 'hf:r:R:d:i:V:v:I:O:', ['help'])
    except getopt.GetoptError, err:
        print str(err)
        usage()
        sys.exit(1)

    dclamp = 'dclamp'
    targetFrequency = 1    # [mV]
    rate = 7000            # [Hz]
    duration = 10          # [s]
    interval = 1           # [s]
    Rm = -1                # [MOhm]
    Vmin = -60             # [mV]
    Vmax = -40             # [mV]
    ai = 0
    ao = 0
    for o,a in opts:
        if o in ('-h','--help'):
            usage()
            sys.exit(0)
        elif o == '-f':
            targetFrequency = float(a)
        elif o == '-R':
            Rm = float(a)
        elif o == '-r':
            rate = float(a)
        elif o == '-i':
            interval = float(a)
        elif o == '-d':
            duration = float(a)
        elif o == '-V':
            Vmin = float(a)
        elif o == '-v':
            Vmax = float(a)
        elif o == '-I':
            ai = int(a)
        elif o == '-O':
            ao = int(a)

    if Vmin >= Vmax:
        print('\n>>> Error: Vmin must be smaller than Vmax. <<<')
        usage()
        sys.exit(1)

    if Rm < 0:
        print('\n>>> Error: you must specify the input resistance of the cell (in MOhm). <<<')
        usage()
        sys.exit(1)

    os.system('kernel_protocol -I ' + str(ai) + ' -O ' + str(ao))
    import scipy.optimize as opt
    Vbal,err,ierr,numfunc = opt.fminbound(frequency_error, Vmin, Vmax,
                                          args = [targetFrequency, Rm, rate, ai, ao, duration, interval, dclamp],
                                          xtol=0.5, maxfun=15, full_output=1, disp=1)

    print('The optimal value of the balanced voltage is %.3f mV (error = %.5f Hz^2).' % (Vbal,err))
    print('The number of performed trials is %.0f.' % numfunc)

if __name__ == '__main__':
    main()
