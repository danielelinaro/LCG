import time
import os
import numpy as np
import tables as tbl

########## Functions that write configuration files ##########

def writeSinusoidallyModulatedOU(f, r0, dr, Rin, tau, dur, type, seed, outfile):
    if type == 'exc':
        g = 0.02/Rin*1e3   # [nS]
    elif type == 'inh':
        g = 0.06/Rin*1e3   # [nS]
    else:
        print("Unknown type [%s]: allowed values are 'exc' or 'inh'." % type)
        return -1
    tau_ms = tau   # [ms]
    tau = tau*1e-3 # [s]
    G = [[2.61,1,0,0,0,0,0,0,0,0,0,1], # "preamble"
         [dur,-3,0,1,tau_ms,0,0,1,seed,2,0,1], # OU
         [0,-3,g**2*tau/2*dr,f,0,g**2*tau/2*r0,0,0,0,3,2,0.5], # sine that sets the variance
         [0,-3,g*tau*dr,f,0,g*tau*r0,0,0,0,3,1,1], # sine that sets the mean
         [1,1,0,0,0,0,0,0,0,0,0,1]] # one second at the end
    return writeStimFile(outfile, G, False)

def writePulsesStimFile(f, dur, amp, N=10, delay=1, withRecovery=True, filename='pulses.stim'):
    """
    Writes a stimulation file containing a series of pulses, with an optional "recovery" pulse.
    
    Parameters:
                 f - frequency of the stimulation
               dur - duration of the stimulation (!!! in ms !!!)
               amp - amplitude of the stimulation
                 N - number of repetitions
             delay - initial and final delay
      withRecovery - whether or not to include a recovery pulse
          filename - file to write to

    Returns:
       The duration of the stimulation.

    """
    dur = dur*1e-3
    stimulus = [[delay,1,0,0,0,0,0,0,5061983,0,0,1]]
    T = 1./f - dur
    for i in range(N):
        stimulus.append([dur,1,amp,0,0,0,0,0,5061983,0,0,1])
        if i != N-1:
            stimulus.append([T,1,0,0,0,0,0,0,5061983,0,0,1])
    if withRecovery:
        stimulus.append([0.5,1,0,0,0,0,0,0,5061983,0,0,1])
        stimulus.append([dur,1,amp,0,0,0,0,0,5061983,0,0,1])
    stimulus.append([delay,1,0,0,0,0,0,0,5061983,0,0,1])
    dur = writeStimFile(filename, stimulus, False)
    print('The total duration of the stimulus is %g sec.' % dur)
    return dur

def writeStimFile(filename, stimulus, addDefaultPreamble=False):
    """
    Writes a generic stimulus file.

    Parameters:
                 filename - the name of the file that will be written.
                 stimulus - a matrix containing the stimulus.
       addDefaultPreamble - whether to add a default preamble composed
                of a 10 ms pulse of -300 pA and a 100 ms pulse of -100 pA.

    Returns:
       The duration of the stimulation.
           
    """
    if type(stimulus) == list and type(stimulus[0]) != list:    # a 1-dimensional list
        stimulus = [stimulus]
    with open(filename,'w') as fid:
        preamble_dur = 0
        if addDefaultPreamble:
            preamble = [[0.5,1,0,0,0,0,0,0,0,0,0,1],
                        [0.01,1,-300,0,0,0,0,0,0,0,0,1],
                        [0.5,1,0,0,0,0,0,0,0,0,0,1],
                        [0.6,1,-100,0,0,0,0,0,0,0,0,1],
                        [1,1,0,0,0,0,0,0,0,0,0,1]]
            for row in preamble:
                preamble_dur = preamble_dur + row[0]
                for value in row:
                    fid.write(str(value)+'\t')
                fid.write('\n')
        for row in stimulus:
            if row[0] == 0 and row[1] > 0:   # don't write lines that have zero duration
                continue
            for value in row:
                fid.write(str(value)+'\t')
            fid.write('\n')
    return preamble_dur + np.sum([row[0] for row in stimulus])
    
def writeGStimFiles(Gexc, Ginh, duration, before, after, outfiles = ['gexc.stim','ginh.stim']):
    G = [[[before,1,0,0,0,0,0,0,0,0,0,1],
          [duration,2,Gexc['m'],Gexc['s'],Gexc['tau'],0,0,1,np.random.poisson(10000),0,0,1],
          [after,1,0,0,0,0,0,0,0,0,0,1]],
         [[before,1,0,0,0,0,0,0,0,0,0,1],
          [duration,2,Ginh['m'],Ginh['s'],Ginh['tau'],0,0,1,np.random.poisson(10000),0,0,1],
          [after,1,0,0,0,0,0,0,0,0,0,1]]]
    if 'seed' in Gexc:
        G[0][1][8] = Gexc['seed']
    if 'seed' in Ginh:
        G[1][1][8] = Ginh['seed']
    for i,filename in enumerate(outfiles):
        writeStimFile(filename,G[i],False)

def writeIPlusBgGConfig(I, Gexc, Ginh, ai, ao, duration, outfile, infile=''):
    if infile == '':
        infile = findConfigurationFile('I_plus_bg_G_template.xml')
        if infile == '':
            print('Unable to locate default configuration file. Aborting...')
            return
    current = [[0.5,1,0,0,0,0,0,0,0,0,0,1],
               [0.01,1,-300,0,0,0,0,0,0,0,0,1],
               [0.5,1,0,0,0,0,0,0,0,0,0,1],
               [0.6,1,-100,0,0,0,0,0,0,0,0,1],
               [1,1,0,0,0,0,0,0,0,0,0,1],
               [duration,1,I,0,0,0,0,0,0,0,0,1],
               [1,1,0,0,0,0,0,0,0,0,0,1]]
    conductance = [[2.61,1,0,0,0,0,0,0,0,0,0,1],
                   [duration,2,1,1,1,0,0,1,0,0,0,1],
                   [1,1,0,0,0,0,0,0,0,0,0,1]]

    writeStimFile('current.stim',current,False)

    conductance[1][2] = Gexc['m']
    conductance[1][3] = Gexc['s']
    conductance[1][4] = Gexc['tau']
    if 'seed' in Gexc:
        conductance[1][8] = Gexc['seed']
    else:
        conductance[1][8] = np.random.poisson(10000)
    writeStimFile('gexc.stim',conductance,False)

    conductance[1][2] = Ginh['m']
    conductance[1][3] = Ginh['s']
    conductance[1][4] = Ginh['tau']
    if 'seed' in Ginh:
        conductance[1][8] = Ginh['seed']
    else:
        conductance[1][8] = np.random.poisson(10000)
    writeStimFile('ginh.stim',conductance,False)

    substituteStrings(infile, outfile, {'<readChannel>0</readChannel>': '<readChannel>'+str(ai)+'</readChannel>',
                                        '<writeChannel>0</writeChannel>': '<writeChannel>'+str(ao)+'</writeChannel>',
                                        '<tend>0</tend>': '<tend>'+str(duration+3.61)+'</tend>'})

def writeSpontaneousConfig(I, G0_exc, sigma_exc, G0_inh, sigma_inh, ai=0, ao=0, duration=10, outfile='spontaneous.xml', infile=''):
    writeIPlusBgGConfig(I, {'m': G0_exc, 's': sigma_exc, 'tau': 5},
                        {'m': G0_inh, 's': sigma_inh, 'tau': 10},
                        ai, ao, duration, outfile, infile)

def writeSinusoidsConfig(bg_current, modulating_current, G0_exc, sigma_exc, G0_inh, sigma_inh,
                         ai=0, ao=0, duration=30, outfile='sinusoids.xml', infile=''):
    import shutil
    writeIPlusBgGConfig(0, {'m': G0_exc, 's': sigma_exc, 'tau': 5, 'seed': 5061983},
                        {'m': G0_inh, 's': sigma_inh, 'tau': 10, 'seed': 7051983},
                        ai, ao, duration, outfile, infile)
    if isinstance(bg_current, float) or isinstance(bg_current,int):
        if bg_current == 0:
            # we have only the sinusoidal modulating current
            current = [[duration,3,modulating_current,'F',0,0,0,0,0,0,0,1],
                       [1,1,0,0,0,0,0,0,0,0,0,1]]
        else:
            # sinusoid on top of a DC current
            current = [[duration,-2,bg_current,0,0,0,0,0,0,1,0,1],
                       [0,-2,modulating_current,'F',0,0,0,0,0,3,1,1],
                       [1,1,0,0,0,0,0,0,0,0,0,1]]
    else:
        if bg_current['mean'] == 0 and bg_current['std'] == 0:
            # we have only the sinusoidal modulating current
            current = [[duration,3,modulating_current,'F',0,0,0,0,0,0,0,1],
                       [1,1,0,0,0,0,0,0,0,0,0,1]]
        else:
            # sinusoid on top of a noisy (OU) current
            current = [[duration,-2,bg_current['mean'],bg_current['std'],bg_current['tau'],0,0,1,5061983,2,0,1],
                       [0,-2,modulating_current,'F',0,0,0,0,0,3,1,1],
                       [1,1,0,0,0,0,0,0,0,0,0,1]]
    os.remove('current.stim')
    shutil.move('gexc.stim','gexc_template.stim')
    shutil.move('ginh.stim','ginh_template.stim')
    writeStimFile('current_template.stim',current,True)

def writeGainModulationConfig(G0_exc, sigma_exc, G0_inh, sigma_inh, ai=0, ao=0, duration=5, outfile='gain_modulation.xml', infile=''):
    writeIPlusBgGConfig(0, {'m': G0_exc, 's': sigma_exc, 'tau': 5},
                        {'m': G0_inh, 's': sigma_inh, 'tau': 10},
                        ai, ao, duration+3, outfile, infile)
    os.remove('current.stim')
    current = [[0.5,1,0,0,0,0,0,0,3532765,0,0,1],
               [0.01,1,-300,0,0,0,0,0,3532765,0,0,1],
               [0.5,1,0,0,0,0,0,0,3532765,0,0,1],
               [0.6,1,-100,0,0,0,0,0,3532765,0,0,1],
               [4,1,0,0,0,0,0,0,3532765,0,0,1],
               [duration,1,'I',0,0,0,0,0,3532765,0,0,1],
               [1,1,0,0,0,0,0,0,3532765,0,0,1]]
    writeStimFile('template.stim',current,False)

def writeFClampConfig(F, I0, duration, gp, gi, gd=0, tau=1, outfile='fclamp.xml', ai=0, ao=0,
                      with_bg=False, G0_exc=0, sigma_exc=0, G0_inh=0, sigma_inh=0):
    if not with_bg:
        infile = findConfigurationFile('fclamp_template.xml')
    else:
        infile = findConfigurationFile('fclamp_plus_bg_G_template.xml')
    if infile == '':
        print('Unable to locate default configuration file. Aborting...')
        return

    writeStimFile('frequency.stim', [duration,1,F,0,0,0,0,0,0,0,0,1], False)

    if with_bg:
        conductance = [duration,2,0,0,0,0,0,0,0,0,0,1]
        conductance[2] = G0_exc
        conductance[3] = sigma_exc
        conductance[4] = 5
        conductance[8] = np.random.poisson(10000)
        writeStimFile('gexc.stim',conductance,False)
        conductance[2] = G0_inh
        conductance[3] = sigma_inh
        conductance[4] = 10
        conductance[8] = np.random.poisson(10000)
        writeStimFile('ginh.stim',conductance,False)

    substituteStrings(infile, outfile, {'TEND': duration, 'AI': ai, 'AO': ao, 'TAU': tau,
                                        'F0': 0.75*F, 'I0': I0, 'GP': gp, 'GI': gi, 'GD': gd})

def writefIStim(Imin,Imax,Istep,noisy=False):
    A = [[0.5,1,0,0,0,0,0,0,3532765,0,0,1],
         [0.01,1,-300,0,0,0,0,0,3532765,0,0,1],
         [0.5,1,0,0,0,0,0,0,3532765,0,0,1],
         [0.6,1,-100,0,0,0,0,0,3532765,0,0,1],
         [1,1,0,0,0,0,0,0,3532765,0,0,1],
         [5,1,500,0,0,0,0,0,3532765,0,0,1],
         [0.1,1,0,0,0,0,0,0,3532765,0,0,1]]
    if noisy:
        A[4,0] = 4
    for k,i in np.arange(Imin,Imax+Istep,Istep):
        A[5,2] = i
        writeStimFile('fi_%02d.stim' % (k+1), A)

def writeNoisyBackgroundConfig(filename='ou.xml', Rm=0, Vm=-57.6, R_exc=7000, tau_exc=5, tau_inh=10, E_exc=0, E_inh=-80):
    ratio = computeRatesRatio(Vm, Rin=Rm, tau_exc, tau_inh, E_exc, E_inh)
    Gm_exc,Gm_inh,Gs_exc,Gs_inh = computeSynapticBackgroundCoefficients(ratio, R_exc, Rin=Rm, tau_exc=tau_exc, tau_inh=tau_inh)
    fid = open(filename,'w')
    fid.write('<dynamicclamp>\n<entities>\n');
    fid.write('<ou>\n');
    fid.write('<G0>%g</G0>\n' % Gm_exc);
    fid.write('<sigma>%g</sigma>\n' % Gs_exc);
    fid.write('<tau>%g</tau>\n' % (tau_exc*1e-3));
    fid.write('<E>%g</E>\n' % E_exc);
    fid.write('</ou>\n');
    fid.write('<ou>\n');
    fid.write('<G0>%g</G0>\n' % Gm_inh);
    fid.write('<sigma>%g</sigma>\n' % Gs_inh);
    fid.write('<tau>%g</tau>\n' % (tau_inh*1e-3));
    fid.write('<E>%g</E>\n' % E_inh);
    fid.write('</ou>\n');
    fid.write('</entities>\n</dynamicclamp>\n');
    fid.close()

########## Utility functions ##########

def makeOutputFilename(prefix='', extension='.out'):
    filename = prefix
    if prefix != '' and prefix[-1] != '_':
        filename = filename + '_'
    now = time.localtime(time.time())
    filename = filename + '%d%02d%02d-%02d%02d%02d' % \
        (now.tm_year, now.tm_mon, now.tm_mday, now.tm_hour, now.tm_min, now.tm_sec)
    if extension[0] != '.':
        extension = '.' + extension
    suffix = ''
    k = 0
    while os.path.exists(filename + suffix + extension):
        k = k+1
        suffix = '_%d' % k
    return filename + suffix + extension

def findConfigurationFile(filename):
    try:
        configsPath = os.environ['CONFIGURATIONS_PATH']
    except:
        configsPath = os.environ['HOME'] + '/configurations'
    path = configsPath + '/' + filename
    if not os.path.exists(path):
        return ''
    return path

def substituteStrings(infile, outfile, rules):
    fid = open(infile,'r')
    text = fid.readlines()
    fid.close()
    fid = open(outfile,'w')
    for line in text:
        for k in rules.keys():
            line = line.replace(str(k), str(rules[k]))
        fid.write(line)
    fid.close()

def computeRatesRatio(Vm=-57.6, g0_exc=50, g0_inh=190, Rin=0, tau_exc=5, tau_inh=10, E_exc=0, E_inh=-80):
    """
    Parameters:
    Vm - membrane potential (mV), at which the mean injected current is 0.
    g0_exc - single synaptic excitatory conductance (pS).
    g0_inh - single synaptic inhibitory conductance (pS).
    Rin - input resistance of the cell(MOhm). If this value is not 0, g0_exc and
         g0_inh are ignored and the values 0.02/Rm and 0.06/Rm
         are used, respectively.
    tau_exc - time constant of excitatory inputs (msec).
    tau_inh - time constant of inhibitory inputs (msec).
    E_exc - reversal potential of excitatory inputs (mV).
    E_inh - reversal potential of inhibitory inputs (mV).
    """ 
    Vm = Vm * 1e-3         # (V)
    Rm = Rm * 1e6          # (Ohm)
    tau_exc = tau_exc*1e-3 # (s)
    tau_inh = tau_inh*1e-3 # (s)
    E_exc = E_exc * 1e-3   # (V)
    E_inh = E_inh * 1e-3   # (V)

    g_exc = g0_exc*1e-12   # (S)
    g_inh = g0_inh*1e-12   # (S)

    if Rm != 0.0:
        g_exc = 0.02 / Rm  # (S)
        g_inh = 0.06 / Rm  # (S)
        
    return (g_inh * tau_inh * (E_inh - Vm)) / (g_exc * tau_exc * (Vm - E_exc))

def computeSynapticBackgroundCoefficients(ratio, R_exc=7000, g0_exc=50, g0_inh=190, N=1, Rin=0, tau_exc=5, tau_inh=10):
    """
    Parameters:
    ratio - ratio between excitatory and inhibitory inputs.
    R_exc - rate of excitatory inputs (Hz).
    g0_exc - single synaptic excitatory conductance (pS).
    g0_inh - single synaptic inhibitory conductance (pS).
    N - number of synaptic contacts.
    Rin - input resistance of the cell(MOhm). If this value is not 0, g0_exc and
         g0_inh are ignored and the values 0.02/Rm and 0.06/Rm
         are used, respectively.
    tau_exc - time constant of excitatory inputs (msec).
    tau_inh - time constant of inhibitory inputs (msec).
    """
    Rm = Rm * 1e6              # (Ohm)
    tau_exc = tau_exc*1e-3     # (s)
    tau_inh = tau_inh*1e-3     # (s)
    if Rm == 0:
        g_exc = N*g0_exc*1e-12   # (S)
        g_inh = N*g0_inh*1e-12   # (S)
    else:
        g_exc = 0.02 / Rm      # (S)
        g_inh = 0.06 / Rm      # (S)

    R_inh = R_exc/ratio;
    G_exc = g_exc * tau_exc * R_exc;
    G_inh = g_inh * tau_inh * R_inh;
    Gm_exc = G_exc * 1e9;        # (nS)
    Gm_inh = G_inh * 1e9;        # (nS)
    D_exc = 0.5 * g_exc**2 * tau_exc**2 * R_exc;
    D_inh = 0.5 * g_inh**2 * tau_inh**2 * R_inh;
    Gs_exc = np.sqrt(D_exc / tau_exc) * 1e9;   # (nS)
    Gs_inh = np.sqrt(D_inh / tau_inh) * 1e9;   # (nS)

    return (Gm_exc,Gm_inh,Gs_exc,Gs_inh)

def computeElectrodeKernel(filename, Kdur=5e-3, interval=[], saveFile=True, fullOutput=False):
    import matplotlib.pyplot as p
    import aec
    p.ion()
    entities,info = loadH5Trace(filename)
    Ksize = int(Kdur/info['dt'])
    Kt = np.arange(0,Kdur,info['dt'])[:Ksize]
    for ntt in entities:
        if ntt['name'] == 'Waveform':
            I = ntt['data']
            stimtimes = np.cumsum(ntt['metadata'][:,0])
            pulse = np.nonzero(ntt['metadata'][:,0] == 0.01)[0][0]
            if len(interval) != 2:
                idx = np.nonzero(ntt['metadata'][:,0] > 5)[0][0]
                interval = stimtimes[idx-1:idx+1]
        elif ntt['units'] == 'mV' :
            V = ntt['data']
    t = np.arange(len(V)) * info['dt']
    idx = np.intersect1d(np.nonzero(t >= interval[0])[0], np.nonzero(t <= interval[1])[0])
    Vn = V[idx]
    V[-Ksize:] = 0
    In = I[idx]
    K,V0 = aec.full_kernel(Vn,In,Ksize,True)

    # Kernels plot
    fig = p.figure(1,figsize=[5,5],facecolor='w',edgecolor=None)
    p.plot(Kt*1e3,K*1e3,'k.-',label='Full')
    p.xlabel('t (ms)')
    p.ylabel('R (MOhm)')
   
    while True:
        try:
            startTail = int(raw_input('Enter index of tail start (1 ms = %d samples): ' % int(1e-3/info['dt'])))
        except:
            continue
        Ke,Km = aec.electrode_kernel(K,startTail,True)
        print('R = %g MOhm.\nV0 = %g mV.' % (np.sum(Ke*1e3),V0))

        # Print kernels and compensated traces
        p.close()
        fig = p.figure(1,figsize=[10,5],facecolor='w',edgecolor=None)
        plt = []
        plt.append(fig.add_axes([0.1,0.1,0.3,0.8]))
        
        plt[0].plot(Kt*1e3,K*1e3,'k.-',label='Full')
        plt[0].set_xlabel('t (ms)')
        plt[0].set_ylabel('R (MOhm)')
        plt.append(fig.add_axes([0.5,0.1,0.4,0.8]))
        plt[0].plot([Kt[0]*1e3,Kt[-1]*1e3],[0,0],'g')
        plt[0].plot(Kt*1e3,Km*1e3,'b',label='Membrane')
        plt[0].plot(Kt[:len(Ke)]*1e3,Ke*1e3,'r',label='Electrode')
        plt[0].set_xlabel('t (ms)')
        plt[0].set_ylabel('R (MOhm)')
        plt[0].legend(loc='best')
        # AEC offline
        ndx = np.intersect1d(np.nonzero(t > max(stimtimes[pulse]-20e-3,0))[0], 
                             np.nonzero(t < min(stimtimes[pulse]+80e-3,t[-1]))[0])
        Vc = aec.compensate(V[ndx],I[ndx],Ke)

        plt[1].plot(t[ndx]-t[ndx[0]], V[ndx], 'k', label='Recorded')
        plt[1].plot(t[ndx]-t[ndx[0]], Vc, 'r', label='Compensated')
        plt[1].set_xlabel('t (ms)')
        plt[1].set_ylabel('V (mV)')
        plt[1].legend(loc='best')
        fig.show()
        # Ask user on what to do
        ok = raw_input('Ke[-1] / max(Ke) = %.2f %%. Ok? [Y/n] ' % (Ke[-1]/np.max(Ke)*100))
        if len(ok) == 0 or ok.lower() == 'y' or ok.lower() == 'yes':
            break
    p.close()

    if saveFile:
        np.savetxt(filename[:-3] + '_kernel.dat', Ke*1e9, '%.10e')
    
    if fullOutput:
        return Ke*1e3,V0,data.values()[0][idx],Vm
    else:
        return Ke*1e3   

########## Data analysis functions ##########

def findSpikes(t, V, thresh=0):
    spks = []
    for i in xrange(len(t)-1):
        if V[i] < thresh and V[i+1] >= thresh:
            spks.append(t[i])
    return np.array(spks)

########## Data loading functions ##########

def loadH5Trace(filename):
    fid = tbl.openFile(filename,mode='r')
    try:
        version = fid.root.Info._v_attrs.version
    except:
        if fid.root.__contains__('Metadata'):
            version = 0
        else:
            version = 1
    fid.close()
    if version == 0:
        return loadH5TraceV0(filename)
    if version == 1:
        return loadH5TraceV1(filename)
    if version == 2:
        return loadH5TraceV2(filename)
    print('Unknown H5 file version (%d).' % version)

def loadH5TraceV0(filename):
    fid = tbl.openFile(filename,mode='r')
    entities = []
    map = {}
    for k,node in enumerate(fid.root.Data):
        id = int(node.name.split('-')[1])
        map[id] = k
        entities.append({'data': node.read(), 'id': id})
    for node in fid.root.Metadata:
        id = int(node.name.split('-')[1])
        entities[map[id]]['metadata'] = node.read()
    for node in fid.root.Parameters:
        id = int(node.name.split('-')[1])
        entities[map[id]]['parameters'] = node.read()
    info = {'dt': fid.root.Misc.Simulation_properties.attrs.dt,
            'tend': fid.root.Misc.Simulation_properties.attrs.tend,
            'version': 0}
    fid.close()
    return entities,info

def loadH5TraceV1(filename):
    fid = tbl.openFile(filename,mode='r')
    entities = []
    for node in fid.root.Data:
        id = int(node.name.split('-')[1])
        entities.append({
                'id': id,
                'data': node.read()})
        for attrName in node.attrs._v_attrnames:
            if len(attrName) > 8 and attrName[:8].lower() == 'metadata':
                entities[-1]['metadata'] = node.attrs[attrName]
            else:
                entities[-1][attrName.lower()] = node.attrs[attrName]
    info = {'dt': fid.root.Misc.Simulation_properties.attrs.dt,
            'tend': fid.root.Misc.Simulation_properties.attrs.tend,
            'version': 1}
    fid.close()
    return entities,info

def loadH5TraceV2(filename):
    fid = tbl.openFile(filename,mode='r')
    try:
        if fid.root.Info._v_attrs.version != 2:
            print('Version not supported: %d.' % version)
            return
    except:
        print('Unknown version.')
        return

    entities = []
    for node in fid.root.Entities:
        entities.append({
                'id': int(node._v_name),
                'data': node.Data.read()})
        try:
            entities[-1]['metadata'] = node.Metadata.read()
        except:
            pass
        for attrName in node._v_attrs._v_attrnames:
            try:
                entities[-1][attrName.lower()] = node._v_attrs[attrName]
            except:
                entities[-1][attrName.lower()] = getattr(node._v_attrs, attrName)
    info = {'dt': fid.root.Info._v_attrs.dt,
            'tend': fid.root.Info._v_attrs.tend,
            'version': fid.root.Info._v_attrs.version}
    fid.close()
    return entities,info
