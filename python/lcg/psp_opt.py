#!/usr/bin/env python

import os
import sys
import getopt
import subprocess as sub
import glob
import numpy as np
import lcg

def deflection_error(weight, target, templateFile, trials=10, window=30e-3, ai=0, ao=0):
    try:
        w = weight[0]
    except:
        w = weight
    lcg.substituteStrings(templateFile, 'psp.xml',
                         {'<weight>0</weight>': '<weight>' + str(w) + '</weight>',
                          'AI': str(ai), 'AO': str(ao)})
    sub.call(lcg.common.prog_name + ' -c psp.xml -V 4 -n ' + str(trials), shell=True)

    # get the file list
    files = glob.glob('*.h5')
    files.sort()
    files = files[-trials:]

    # read the first file to allocate memory
    entities,info = lcg.loadH5Trace(files[0])
    for ntt in entities:
        if ntt['name'] == 'RealNeuron' or ntt['id'] == 4:
            post = ntt
        elif ntt['name'] == 'LIFNeuron':
            pre = ntt
        elif ntt['name'] == 'Waveform':
            t0 = ntt['metadata'][0][0]
    #if max(pre['data']) < -50:    # no spike in the presynaptic
    #    print('>>> No spike in the presynaptic neuron. <<<')
    #    sys.exit(1)

    # allocate memory
    t = np.arange(0, info['dt']*len(post['data']), info['dt'])
    idx = np.nonzero(t < t0)[0]
    dV = np.zeros((trials,len(post['data'])))
    dV[0,:] = post['data'] - np.mean(post['data'][idx])
    for k in range(1,trials):
        entities,info = lcg.loadH5Trace(files[k])
        for ntt in entities:
            if ntt['name'] == 'RealNeuron' or ntt['id'] == 4:
                dV[k,:] = ntt['data'] - np.mean(ntt['data'][idx])
                break
    dV = np.mean(dV,0)
    idx = np.intersect1d(np.nonzero(t>t0)[0], np.nonzero(t<t0+window)[0])
    deflection = np.max(np.abs(dV[idx]))
    print('weight = %.3f -> deflection = %.3f mV' % (weight,deflection))
    return (deflection - target)**2

def usage():
    print('\nUsage: %s -e/-i [option <value>]' % os.path.basename(sys.argv[0]))
    print('\nwhere options are:\n')
    print('     -h    display this help message and exit.')
    print('     -e    use the template file for excitatory post-synaptic potentials.')
    print('     -i    use the template file for inhibitory post-synaptic potentials.')
    print('     -d    target deflection (default 1 mV).')
    print('     -M    maximum value of the synaptic weight (default 500).')
    print('     -w    time window for extracting the peak of the PSP (default 30 ms).')
    print('     -t    number of trials for averaging the amplitude of the PSP (default 10).')
    print('     -I    input channel (default 0).')
    print('     -O    output channel (default 0).\n')
    print('')

def main():
    try:
        opts,args = getopt.getopt(sys.argv[1:], 'heid:M:w:t:I:O:', ['help'])
    except getopt.GetoptError, err:
        print str(err)
        usage()
        sys.exit(1)

    targetDeflection = 1    # [mV]
    minWeight = 0           # [a.u.]
    maxWeight = 50          # [a.u.]
    window = 30             # [ms]
    trials = 10
    ai = 0
    ao = 0
    try:
        configurationsDir = os.environ['CONFIGURATIONS_PATH']
    except:
        configurationsDir = os.environ['HOME'] + '/configurations'
    templateFile = None

    for o,a in opts:
        if o in ('-h','--help'):
            usage()
            sys.exit(0)
        elif o == '-e':
            if templateFile != None:
                print('Cannot specify -e and -i at the same time.')
                usage()
                sys.exit(2)
            templateFile = configurationsDir + '/epsp_template.xml'
        elif o == '-i':
            if templateFile != None:
                print('Cannot specify -i and -e at the same time.')
                usage()
                sys.exit(3)
            templateFile = configurationsDir + '/ipsp_template.xml'
        elif o == '-d':
            targetDeflection = float(a)
        elif o == '-M':
            maxWeight = float(a)
        elif o == '-w':
            window = float(a)
        elif o == '-t':
            trials = int(a)
        elif o == '-I':
            ai = int(a)
        elif o == '-O':
            ao = int(a)

    if templateFile == None:
        print('You must specify either -e or -i.')
        usage()
        sys.exit(4)

    if not os.path.isfile(templateFile):
        print('The template file [%s] does not exist.' % templateFile)
        usage()
        sys.exit(5)

    sub.call('lcg kernel -I ' + str(ai) + ' -O ' + str(ao))
    import scipy.optimize as opt
    weight,err,ierr,numfunc = opt.fminbound(deflection_error, minWeight, maxWeight,
                                            args = [targetDeflection, templateFile, trials, window*1e-3, ai, ao],
                                            xtol=0.5, maxfun=100, full_output=1, disp=1)

    print('The optimal value of the weight is %.3f (error = %.5f mV^2).' % (weight,err))
    print('The number of performed trials is %.0f.' % numfunc)

if __name__ == '__main__':
    main()
