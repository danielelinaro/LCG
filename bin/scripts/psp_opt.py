#!/usr/bin/env python

import os
import sys
import glob
import numpy as np
import dlutils as dl

def deflection_error(weight, target, window=30e-3, dclamp='dclamp'):
    filename = 'psp.xml'
    dl.substituteStrings('psp_template.xml', filename,
                         {'<weight>W</weight>': '<weight>' + str(weight) + '</weight>'})
    os.system('sleep 1')
    os.system(dclamp + ' -c ' + filename)     # run dclamp
    filename = glob.glob('*.h5')[-1]    # take the last saved file
    print('Reading from [%s].' % filename)
    entities,info = dl.loadH5Trace(filename)
    t = np.arange(0, info['dt']*len(entities[0]['data']), info['dt'])
    for ntt in entities:
        #if ntt.name == 'RealNeuron':
        if ntt['id'] == '0004':
            post = ntt
        elif ntt['name'] == 'LIFNeuron':
            pre = ntt
        elif ntt['name'] == 'Waveform':
            t0 = ntt['metadata'][0][0]
    if max(pre['data']) < -30:    # no spike in the presynaptic
        return target**2
    Vbase = np.mean(post['data'][t<t0])    # baseline voltage of the postsynaptic neuron
    idx = np.intersect1d(np.nonzero(t>t0)[0], np.nonzero(t<t0+window)[0])
    deflection = np.max(np.abs(post['data'][idx] - Vbase))
    print('weight = %.0f -> deflection = %.3f mV' % (weight,deflection))
    return (deflection - target)**2

def usage():
    print('\nUsage: %s [option <value>]' % sys.argv[0])
    print('\nwhere options are:\n')
    print('     -d    target deflection (default 1 mV)')
    print('     -M    maximum value of the synaptic weight (default 500)')
    print('     -w    time window for extracting the peak of the PSP (default 30 ms)\n')

def main():
    import getopt

    try:
        opts,args = getopt.getopt(sys.argv[1:], "hd:M:w:", ["help", "output="])
    except getopt.GetoptError, err:
        print str(err)
        usage()
        sys.exit(1)

    dclamp = '../bin/dclamp'
    targetDeflection = 1    # [mV]
    minWeight = 0           # [a.u.]
    maxWeight = 500         # [a.u.]
    window = 30             # [ms]
    
    for o,a in opts:
        if o == '-h':
            usage()
            sys.exit(0)
        elif o == '-d':
            targetDeflection = float(a)
        elif o == '-M':
            maxWeight = float(a)
        elif o == '-w':
            window = float(w)

    import scipy.optimize as opt
    weight,err,ierr,numfunc = opt.fminbound(deflection_error, minWeight, maxWeight,
                                            args = [targetDeflection, window*1e-3, dclamp],
                                            xtol=1, maxfun=60, full_output=1, disp=1)

    print('The optimal value of the weight is %.0f (error = %.5f mV^2).' % (weight,err))
    print('The number of performed trials is %.0f.' % numfunc)

if __name__ == '__main__':
    main()
