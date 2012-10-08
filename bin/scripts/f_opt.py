#!/usr/bin/env python

import os
import sys
import glob
import numpy as np
import dlutils as dl

def frequency_error(Vbal, target, Rm, R_exc, duration=10, dclamp='dclamp'):
    ratio = dl.computeRatesRatio(Vbal, Rm)
    G0_exc,G0_inh,sigma_exc,sigma_inh = dl.computeSynapticBackgroundCoefficients(ratio[0], Rm, R_exc)
    dl.writeSpontaneousConfig(0, G0_exc, sigma_exc, G0_inh, sigma_inh, duration, outfile='spontaneous.xml')
    os.system(dclamp + ' -c spontaneous.xml -V 4')     # run dclamp
    files = glob.glob('*.h5')
    files.sort()
    files = files[-1]
    entities,info = dl.loadH5Trace(files)
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
    spks = dl.findSpikes(t,V,-20)
    freq = float(len(spks)) / duration
    print('Balanced voltage: %.2f mV.' % Vbal)
    print('Spontaneous firing frequency: %.3f Hz (%d spikes).' % (freq, len(spks)))
    print('Error = %g Hz^2.' % (freq-target)**2)
    return (freq - target)**2

def usage():
    print('\nUsage: %s -e/-i [option <value>]' % sys.argv[0])
    print('\nwhere options are:\n')
    print('     -f    target frequency (default 1 Hz)')
    print('     -r    background synaptic rate (default 7000 Hz)')
    print('     -R    input resistance of the cell in MOhm')
    print('     -d    duration of the stimulation (default 10 s)\n')

def main():
    import getopt

    try:
        opts,args = getopt.getopt(sys.argv[1:], "hf:r:R:d:", ["help", "output="])
    except getopt.GetoptError, err:
        print str(err)
        usage()
        sys.exit(1)

    dclamp = 'dclamp'
    targetFrequency = 1    # [mV]
    rate = 7000            # [Hz]
    duration = 10          # [s]
    Rm = -1                # [MOhm]

    for o,a in opts:
        if o == '-h':
            usage()
            sys.exit(0)
        elif o == '-f':
            targetFrequency = float(a)
        elif o == '-R':
            Rm = float(a)
        elif o == '-r':
            rate = float(a)
        elif o == '-d':
            duration = float(a)

    if Rm < 0:
        print('Need to specify the input resistance.')
        usage()
        sys.exit(1)

    import scipy.optimize as opt
    Vbal,err,ierr,numfunc = opt.fminbound(frequency_error, -55, -40,
                                          args = [targetFrequency, Rm, rate, duration, dclamp],
                                          xtol=0.5, maxfun=15, full_output=1, disp=1)

    print('The optimal value of the balanced voltage is %.3f mV (error = %.5f Hz^2).' % (Vbal,err))
    print('The number of performed trials is %.0f.' % numfunc)

if __name__ == '__main__':
    main()
