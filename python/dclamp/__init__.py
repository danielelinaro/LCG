"""

dclamp
======

Python tools for use with dynclamp, command-line electrophysiology

Documentation is available in the .

For a list of available names, try dir(dclamp).


---------------------

$Id: __init__.py,v 0.1 2013/01/16 15:43:47 joao$

"""

from io_utils import load
import AEC 

__all__ = ['load','computeElectrodeKernel']

def computeElectrodeKernel(entities, info, Kdur=5e-3, interval=[], saveFile=True, fullOutput=False):
    import numpy as np
    import matplotlib.pyplot as p
    p.ion()
#    entities,info = load(filename)
    Ksize = int(Kdur/info['dt'])
    Kt = np.arange(0,Kdur,info['dt'])[:Ksize]
    for ntt in entities:
        if 'Waveform' in ntt['name']:
            I = ntt['data']
            stimtimes = np.cumsum(ntt['metadata'][:,0])
            pulse = np.nonzero(ntt['metadata'][:,0] == 0.01)[0][0]
            if len(interval) != 2:
                idx = np.nonzero(ntt['metadata'][:,0] > 5)[0][0]
                interval = stimtimes[idx-1:idx+1]
        elif 'RealNeuron' in ntt['name'] or 'AnalogInput' in ntt['name']:
            V = ntt['data']
    t = np.arange(len(V)) * info['dt']
    idx = np.intersect1d(np.nonzero(t >= interval[0])[0], np.nonzero(t <= interval[1])[0])
    Vn = V[idx]
    V[-Ksize:] = 0
    In = I[idx]
    K,V0 = AEC.full_kernel(Vn,In,Ksize,True)

    # Voltage and current plot
    p.figure()
    lbls = ['I (pA)','V (mV)']
    for k,y in enumerate([I,V]):
        p.subplot(1,2,k+1)
        p.plot(t,y,'k')
        yl = p.ylim()
        for x in interval:
            p.plot([x,x],yl,'r--')
        p.axis([t[0],t[-1],yl[0],yl[1]])
        p.xlabel('t (s)')
        p.ylabel(lbls[k])

    # Kernels plot
    p.figure()
    p.plot(Kt*1e3,K*1e3,'k.-',label='Full')
    p.xlabel('t (ms)')
    p.ylabel('R (MOhm)')

    while True:
        try:
            startTail = int(raw_input('Enter index of tail start (1 ms = %d samples): ' % int(1e-3/info['dt'])))
        except:
            continue
        Ke,Km = AEC.electrode_kernel(K,startTail,True)
        print('R = %g MOhm.\nV0 = %g mV.' % (np.sum(Ke*1e3),V0))
        ok = raw_input('Ke[-1] / max(Ke) = %.2f %%. Ok? [Y/n] ' % (Ke[-1]/np.max(Ke)*100))
        if len(ok) == 0 or ok.lower() == 'y' or ok.lower() == 'yes':
            break

    p.plot([Kt[0]*1e3,Kt[-1]*1e3],[0,0],'g')
    p.plot(Kt*1e3,Km*1e3,'b',label='Membrane')
    p.plot(Kt[:len(Ke)]*1e3,Ke*1e3,'r',label='Electrode')
    p.xlabel('t (ms)')
    p.ylabel('R (MOhm)')
    p.legend(loc='best')
    p.ioff()

    ndx = np.intersect1d(np.nonzero(t > max(stimtimes[pulse]-20e-3,0))[0], 
                         np.nonzero(t < min(stimtimes[pulse]+80e-3,t[-1]))[0])
    Vc = AEC.AEC_compensate(V[ndx],I[ndx],Ke)
    p.figure()
    p.plot(t[ndx]-t[ndx[0]], V[ndx], 'k', label='Recorded')
    p.plot(t[ndx]-t[ndx[0]], Vc, 'r', label='Compensated')
    p.xlabel('t (ms)')
    p.ylabel('V (mV)')
    p.legend(loc='best')

    p.show()

    if saveFile:
        np.savetxt(filename[:-3] + '_kernel.dat', Ke*1e9, '%.10e')
    
    if fullOutput:
        return Ke*1e3,V0,data.values()[0][idx],Vm
    else:
        return Ke*1e3   
