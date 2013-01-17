import time
import os
import numpy as np
import tables as tbl

__all__ = ['findConfigurationFile', 'writeIPlusBgGConfig', 
           'writeSinusoidsConfig', 'writeFClampConfig', 'writeSpontaneousConfig', 'substituteStrings',
           'writefIStim','writeNoisyBackgroundConfig', 'computeRatesRatio',
           'computeSynapticBackgroundCoefficients','makeOutputFilename',
           'loadH5Trace','loadH5TraceV0','loadH5TraceV1','loadH5TraceV2',
           'computeElectrodeKernel']

def findConfigurationFile(filename):
    try:
        configsPath = os.environ['CONFIGURATIONS_PATH']
    except:
        configsPath = os.environ['HOME'] + '/configurations'
    path = configsPath + '/' + filename
    if not os.path.exists(path):
        return ''
    return path

def writeIPlusBgGConfig(I, Gexc, Ginh, duration, outfile, infile=''):
    if infile == '':
        infile = findConfigurationFile('I_plus_bg_G_template.xml')
        if infile == '':
            print('Unable to locate default configuration file. Aborting...')
            return
    current = [[0.5,1,0,0,0,0,0,0,3532765,0,0,1],
               [0.01,1,-300,0,0,0,0,0,3532765,0,0,1],
               [0.5,1,0,0,0,0,0,0,3532765,0,0,1],
               [0.6,1,-100,0,0,0,0,0,3532765,0,0,1],
               [1,1,0,0,0,0,0,0,3532765,0,0,1],
               [duration,1,I,0,0,0,0,0,3532765,0,0,1],
               [1,1,0,0,0,0,0,0,3532765,0,0,1]]
    conductance = [[2.61,1,0,0,0,0,0,0,3532765,0,0,1],
                   [duration,2,1,1,1,0,0,0,3532765,0,0,1],
                   [1,1,0,0,0,0,0,0,3532765,0,0,1]]
    fid = open('current.stim','w')
    for row in current:
        for value in row:
            fid.write(str(value)+'\t')
        fid.write('\n')
    fid.close()
    conductance[1][2] = Gexc['m']
    conductance[1][3] = Gexc['s']
    conductance[1][4] = Gexc['tau']
    fid = open('gexc.stim','w')
    for row in conductance:
        for value in row:
            fid.write(str(value)+'\t')
        fid.write('\n')
    fid.close()
    conductance[1][2] = Ginh['m']
    conductance[1][3] = Ginh['s']
    conductance[1][4] = Ginh['tau']
    fid = open('ginh.stim','w')
    for row in conductance:
        for value in row:
            fid.write(str(value)+'\t')
        fid.write('\n')
    fid.close()
    substituteStrings(infile, outfile, {'<tend>0</tend>': '<tend>'+str(duration+3.61)+'</tend>'})

def writeSpontaneousConfig(I, G0_exc, sigma_exc, G0_inh, sigma_inh, duration=10, outfile='spontaneous.xml', infile=''):
    writeIPlusBgGConfig(I, {'m': G0_exc, 's': sigma_exc, 'tau': 5},
                        {'m': G0_inh, 's': sigma_inh, 'tau': 10},
                        duration, outfile, infile)

def writeSinusoidsConfig(I0, I1, G0_exc, sigma_exc, G0_inh, sigma_inh, duration=30, outfile='sinusoids.xml', infile=''):
    writeIPlusBgGConfig(0, {'m': G0_exc, 's': sigma_exc, 'tau': 5},
                        {'m': G0_inh, 's': sigma_inh, 'tau': 10},
                        duration, outfile, infile)
    current = [[0.5,1,0,0,0,0,0,0,3532765,0,0,1],
               [0.01,1,-300,0,0,0,0,0,3532765,0,0,1],
               [0.5,1,0,0,0,0,0,0,3532765,0,0,1],
               [0.6,1,-100,0,0,0,0,0,3532765,0,0,1],
               [1,1,0,0,0,0,0,0,3532765,0,0,1],
               [duration,-2,I0,0,0,0,0,0,3532765,1,0,1],
               [0,-2,I1,'F',0,0,0,0,3532765,1,0,1],
               [1,1,0,0,0,0,0,0,3532765,0,0,1]]
    fid = open('current.stim','w')
    for row in current:
        for value in row:
            fid.write(str(value)+'\t')
        fid.write('\n')
    fid.close()

def writeGainModulationConfig(G0_exc, sigma_exc, G0_inh, sigma_inh, duration=5, outfile='gain_modulation.xml', infile=''):
    preambleDuration = 3
    writeIPlusBgGConfig(0, {'m': G0_exc, 's': sigma_exc, 'tau': 5},
                        {'m': G0_inh, 's': sigma_inh, 'tau': 10},
                        duration+preambleDuration, outfile, infile)
    current = [[0.5,1,0,0,0,0,0,0,3532765,0,0,1],
               [0.01,1,-300,0,0,0,0,0,3532765,0,0,1],
               [0.5,1,0,0,0,0,0,0,3532765,0,0,1],
               [0.6,1,-100,0,0,0,0,0,3532765,0,0,1],
               [1+preambleDuration,1,0,0,0,0,0,0,3532765,0,0,1],
               [duration,1,'I',0,0,0,0,0,3532765,0,0,1],
               [1,1,0,0,0,0,0,0,3532765,0,0,1]]
    fid = open('template.stim','w')
    for row in current:
        for value in row:
            fid.write(str(value)+'\t')
        fid.write('\n')
    fid.close()

def writeFClampConfig(G0_exc, sigma_exc, G0_inh, sigma_inh, F, I0, gp, gi, duration, outfile='fclamp.xml', infile=''):
    if infile == '':
        infile = findConfigurationFile('fclamp_template.xml')
        if infile == '':
            print('Unable to locate default configuration file. Aborting...')
            return
    frequency = [duration,1,F,0,0,0,0,0,3532765,0,0,1]
    conductance = [duration,2,0,0,0,0,0,0,3532765,0,0,1]

    fid = open('frequency.stim','w')
    for value in frequency:
        fid.write(str(value)+'\t')
    fid.write('\n')
    fid.close()

    conductance[2] = G0_exc
    conductance[3] = sigma_exc
    conductance[4] = 0.005
    fid = open('gexc.stim','w')
    for value in conductance:
        fid.write(str(value)+'\t')
    fid.write('\n')
    fid.close()

    conductance[2] = G0_inh
    conductance[3] = sigma_inh
    conductance[4] = 0.01
    fid = open('ginh.stim','w')
    for value in conductance:
        fid.write(str(value)+'\t')
    fid.write('\n')
    substituteStrings(infile, outfile, {'<tend>0</tend>': '<tend>'+str(duration)+'</tend',
                                        'I0': I0, 'GP': gp, 'GI': gi})

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

def writefIStim(Imin,Imax,Istep,noisy=False):
    I = np.arange(Imin,Imax+Istep,Istep)
    A = np.array([[0.5,1,0,0,0,0,0,0,3532765,0,0,1],
                  [0.01,1,-300,0,0,0,0,0,3532765,0,0,1],
                  [0.5,1,0,0,0,0,0,0,3532765,0,0,1],
                  [0.6,1,-100,0,0,0,0,0,3532765,0,0,1],
                  [1,1,0,0,0,0,0,0,3532765,0,0,1],
                  [5,1,500,0,0,0,0,0,3532765,0,0,1],
                  [0.1,1,0,0,0,0,0,0,3532765,0,0,1]])
    if noisy:
        A[4,0] = 4
    for k,i in enumerate(I):
        A[5,2] = i
        np.savetxt('fi_%02d.stim' % (k+1), A, '%10g')

def writeNoisyBackgroundConfig(filename='ou.xml', Rm=0, Vm=-57.6, R_exc=7000, tau_exc=5, tau_inh=10, E_exc=0, E_inh=-80):
    ratio = computeRatesRatio(Vm, Rm, tau_exc, tau_inh, E_exc, E_inh)
    Gm_exc,Gm_inh,Gs_exc,Gs_inh = computeSynapticBackgroundCoefficients(ratio, Rm, R_exc, tau_exc, tau_inh)
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

def computeRatesRatio(Vm=-57.6, Rm=0, tau_exc=5, tau_inh=10, E_exc=0, E_inh=-80):
    """
    ratio = computeRatesRatio(Vm, Rm, tau_exc, tau_inh)
    
    Parameters:
    Vm - membrane potential (mV), default -57.6.
    Rm - membrane resistance (MOhm), default 0, which means to use biophysically realistic
         values for single channel conductances (50 and 190 pS for AMPA and GABA, respectively).
    tau_exc - time constant of excitatory inputs (msec), default 5.
    tau_inh - time constant of inhibitory inputs (msec), default 10.
    E_exc - reversal potential of excitatory inputs (mV), default 0.
    E_inh - reversal potential of inhibitory inputs (mV), default -80.
    """ 
    Vm = Vm * 1e-3         # (V)
    Rm = Rm * 1e6          # (Ohm)
    tau_exc = tau_exc*1e-3 # (s)
    tau_inh = tau_inh*1e-3 # (s)
    E_exc = E_exc * 1e-3   # (V)
    E_inh = E_inh * 1e-3   # (V)
    
    if Rm == 0:
        g_exc = 50e-12     # (S)
        g_inh = 190e-12    # (S)
    else:
        g_exc = 0.02 / Rm  # (S)
        g_inh = 0.06 / Rm  # (S)
        
    return (g_inh * tau_inh * (E_inh - Vm)) / (g_exc * tau_exc * (Vm - E_exc))

def computeSynapticBackgroundCoefficients(ratio, Rm=0, R_exc=7000, tau_exc=5, tau_inh=10):
    """
    Gm_exc,Gm_inh,Gs_exc,Gs_inh = 
         computeSynapticBackgroundCoefficients(ratio, Rm, R_exc, tau_exc, tau_inh)
    Parameters:
    ratio - ratio between excitatory and inhibitory inputs.
    Rm - membrane resistance (MOhm).
    R_exc - rate of excitatory inputs (Hz).
    tau_exc - time constant of excitatory inputs (msec), default 5.
    tau_inh - time constant of inhibitory inputs (msec), default 10.
    """
    Rm = Rm * 1e6              # (Ohm)
    tau_exc = tau_exc*1e-3     # (s)
    tau_inh = tau_inh*1e-3     # (s)
    if Rm == 0:
        g_exc = 50e-12         # (S)
        g_inh = 190e-12        # (S)
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
    print('H5 file is saved according to version #%d.' % version)
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
                'id': node._v_name,
                'data': node.Data.read()})
        try:
            entities[-1]['metadata'] = node.Metadata.read()
        except:
            pass
        for attrName in node._v_attrs._v_attrnames:
            entities[-1][attrName.lower()] = node._v_attrs[attrName]
    info = {'dt': fid.root.Info._v_attrs.dt,
            'tend': fid.root.Info._v_attrs.tend,
            'version': fid.root.Info._v_attrs.version}
    fid.close()
    return entities,info
def computeElectrodeKernel(filename, Kdur=5e-3, interval=[], saveFile=True, fullOutput=False):
    import AECoffline as aec
    import matplotlib.pyplot as p
    p.ion()
    entities,info = loadH5Trace(filename)
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
    K,V0 = aec.full_kernel(Vn,In,Ksize,True)

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
        Ke,Km = aec.electrode_kernel(K,startTail,True)
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
    Vc = aec.AEC_compensate(V[ndx],I[ndx],Ke)
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


