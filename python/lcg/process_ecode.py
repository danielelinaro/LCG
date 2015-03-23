import lcg
from glob import glob
import numpy as np
import os
import sys
import matplotlib.pylab as plt 
import aec
from scipy.signal import argrelmax
from scipy.interpolate import interp1d
plt.rc('font',**{'family':'sans-serif','sans-serif':['Arial'], 'size': 8})
plt.rc('axes', linewidth =  1)
plt.rc("xtick", direction="out")
plt.rc("ytick", direction="out")
plt.rc("ytick", direction="out")

def gatherH5files(folder):
    files = glob('{0}/*.h5'.format(folder))
    kfiles = glob('{0}/*_kernel*.dat'.format(folder))
    return files, kfiles

def argfindspks(data, threshold=-20,deadwindow=30):
    ''' Extracts the indexes of the peaks in the data with threshold crossing.
    Uses a dead window/minimum peak distance.
    '''
    N = len(data)
    ii = np.arange(0,N)
    # threshold crossing
    dx = data > threshold
    idx=ii[dx][np.diff(ii[dx])>1]
    idx = np.append(idx,[ii[dx][-1]])
    # find peaks using the dead window
    index = []
    for ii in idx:
        lower = ii - deadwindow
        upper = ii + deadwindow
        if  lower < 0:
            lower = 0
        if upper > N:
            upper = N
        index.append(lower + np.argmax(data[lower:upper]))
    return np.array(index)

def compensateVoltage(ent, Ke, entNames = ['AnalogInput', 'Waveform']):
    for e in ent:
        if e['name'] in [entNames[0]]:
            V =  e['data']
        if e['name'] in [entNames[1]]: 
            I =  e['data']
            metadata =  e['metadata']
    V = aec.compensate(V, I, Ke)
    return V, I, metadata

def analyseAPprotocol(folder, tpre = 5,  tpost = 20, ax = None):
    files, kfiles = gatherH5files(folder)
    # Uses the first kernel
    Ke =  np.loadtxt(kfiles[0]) / 1e9
    V =  []
    I =  []
    idx = []
    ktrials =  [ os.path.basename(k).split('_')[0] for k in kfiles]
    print('Analysing spike shapes...')
    for f in files:
        ent, info =  lcg.loadH5Trace(f)
        if not os.path.basename(f).split('.')[0] in ktrials: 
            try:
                inset_samples
            except NameError:
                inset_samples = np.arange(np.floor(-(tpre*1e-3)/info['dt']),
                                          np.floor(+(tpost*1e-3) / info['dt']),
                                          dtype = int)
                # time_actual =  np.linspace( - tpre, tpost, len(inset_samples))
                time =  np.linspace( - tpre, tpost, len(inset_samples))
            tmpV, tmpI, meta = compensateVoltage(ent, Ke, ['AnalogInput','Waveform'])
            spks =  argfindspks(tmpV,-20)
            if len(spks) == 1:
                # f = interp1d(time_actual, tmpV[spks + inset_samples], 'cubic')
                # V.append(f(time))
                V.append(tmpV[spks + inset_samples])
                I.append(tmpI[spks + inset_samples])
    V =  np.vstack(V).T
    I =  np.vstack(I).T
    dt =  info['dt'] * 1e3
    dV3 =  np.diff(V, 3, 0)/(dt ** 3)
    dV1 =  np.diff(V, 1, 0)/dt
    dV3Thresh = 10000
    # Not counting for the extra points resulting from the diff
    idx =  np.array([np.where(dV3[::,ii]>dV3Thresh)[0][0] for ii in range(dV3.shape[1])]) + 3
    idxmean =  np.where(np.mean(dV3, 1)>dV3Thresh)[0][0] + 3
    mV =  np.mean(V, 1)
    # mdV1 =  np.mean(dV1[1:,], 1)
    # ax.plot(mV[1:],mdV1, color = 'r', lw = 1)
    # ax.plot(mV[idxmean + 1] + [ - 5,+ 5], mdV1[idxmean] + [0, 0], 'k--')
    ax.plot(time, V, color = 'gray', lw = 0.8)
    [ax.plot(time[ii], V[ii, jj], 'ro',
             markerfacecolor = 'gray',
             markersize = 2) for jj, ii in enumerate(idx)]
    ax.plot(time,mV, 'r', lw = 1)
    ax.axis('tight')
    ax.set_xticks(np.unique(np.linspace(-tpre, tpost, 10).astype(int)))
    ax.set_yticks(np.unique(np.linspace(np.min(mV), max(mV), 5).astype(int)))

    ax.plot(time[np.mean(idx)] + [0, 0],ax.get_ylim(),'k--')
    tmp = mV[np.mean(idx)] + [0, 0]
    ax.plot([ - tpre, tpost],tmp, 'k--')
    # Scale in right corner
    ax.set_xticks(np.linspace(-tpre, tpost, 12).astype(int))
    ax.set_yticks(np.linspace(min(mV), max(mV), 5).astype(int))
    tmpx = ax.get_xticks()
    tmpy = ax.get_yticks()
    
    ax.plot(tmpx[ -4:-2], tmpy[ - 2] + [0, 0], 'k')
    ax.plot(tmpx[ - 4] + [0, 0], tmpy[ - 2::], 'k')
    ax.text(tmpx[ - 4], tmpy[-2] - 2, '{0}ms'.format(np.diff(tmpx[0: 2])[0]),
            va = 'top', ha = 'left', fontsize = 7)
    ax.text(tmpx[ - 4], tmpy[-1], '{0}mV'.format(np.diff(tmpy[-2::])[0]),
            va = 'top', ha = 'right', fontsize = 7, rotation =90)

    return mV[np.mean(idx)]

def analyseVIprotocol(folder, ax = None):
    files, kfiles = gatherH5files(folder)
    Ke =  np.loadtxt(kfiles[0]) / 1e9
    V =  []
    I =  []
    idx = []
    ktrials =  [ os.path.basename(k).split('_')[0] for k in kfiles]
    print('Analysing hyperpolarizing current steps...')
    for f in files:
        ent, info =  lcg.loadH5Trace(f)
        if not os.path.basename(f).split('.')[0] in ktrials:
            tmpV, tmpI, meta = compensateVoltage(ent, Ke, ['AnalogInput', 'Waveform'])
            V.append(tmpV)
            I.append(tmpI)
    time = np.linspace( 0, info['tend'], len(tmpV))
    prot_time = np.cumsum(meta[::, 0])
    idx =  np.where(time > prot_time[-3] - .2)[0]
    V =  np.vstack(V).T
    I =  np.vstack(I).T
    ax[1].plot(time[idx], V[idx,::], 'k', clip_on = False)
    ax[0].plot(time[idx], I[idx,::], 'k', clip_on = False)
    # Scale in left corner
    for a in ax:
        a.axis('tight')
        a.clipbox = False
        a.set_xlim([time[idx[0]], time[idx[-1]]])

        tmpx = a.get_xlim()
        tmpy = a.get_ylim()
        a.set_yticks(np.linspace(tmpy[0],tmpy[-1], 4).astype(int))
        a.set_xticks(np.round(np.linspace(tmpx[0],tmpx[ -1], 4), 3) )
    print time[idx[0]]  
    tmpx = ax[1].get_xticks()   
    tmpy = ax[1].get_yticks()

    ax[1].plot(tmpx[ -1] + [0, 0], tmpy[:2] + 2, 'k', clip_on=False)
    ax[1].text(tmpx[ -1] + 0.05, tmpy[ 1], '{0}mV'.format(np.diff(tmpy[-2::])[0]),
            va = 'top', ha = 'left', fontsize = 7, rotation =90)
    ax[1].plot(tmpx[-2:], tmpy[0] + [2, 2], 'k', clip_on=False)
    ax[1].text(tmpx[ -2] , tmpy[0] + 3, '{0}s'.format(np.diff(tmpx[0: 2])[0]),
               va = 'bottom', ha = 'left', fontsize = 7)
    tmpx = ax[0].get_xticks()
    tmpy = ax[0].get_yticks()
    ax[0].plot(tmpx[ -1] + [0, 0], tmpy[:2], 'k', clip_on=False)
    ax[0].text(tmpx[ -1] + 0.05, tmpy[ 1], '{0}pA'.format(np.diff(tmpy[-2::])[0]),
            va = 'top', ha = 'left', fontsize = 7, rotation =90)
    return None
    # 

def analyseTAUprotocol(folder, ax = None):
    files, kfiles = gatherH5files(folder)
    Ke =  np.loadtxt(kfiles[0]) / 1e9
    V =  []
    I =  []
    spks = []
    ktrials =  [ os.path.basename(k).split('_')[0] for k in kfiles]
    print('Analysing tau protocol...')
    for f in files:
        ent, info =  lcg.loadH5Trace(f)
        if not os.path.basename(f).split('.')[0] in ktrials:
            tmpV, tmpI, meta = compensateVoltage(ent, Ke, ['AnalogInput', 'Waveform'])
            V.append(tmpV)
            I.append(tmpI)
    time = np.linspace( 0, info['tend'], len(tmpV))
    prot_time = np.cumsum(meta[::, 0])
    idx =  np.where((time > prot_time[0] - 0.0055) & (time < prot_time[1] + 0.1))[0]
    fitidx =  np.where((time > prot_time[1] + 0.0007) & (time < prot_time[1] + 0.08))[0]
    V =  np.vstack(V).T
    I =  np.vstack(I).T
    ax.plot(time[idx], V[idx,], color = 'gray', lw = 0.6)
    ax.plot(time[idx], np.mean(V[idx,], 1), color = 'k', lw =.7)
    func = lambda x, a, b, c, d, e: a * (1-np.exp( -x / b )) + c * (1-np.exp( -x / d )) + e

    from scipy.optimize import curve_fit

    popt, pcov = curve_fit(func, np.reshape(np.repeat(time - time[fitidx[0]],V.shape[1]),V.shape)[fitidx,: ].flatten(), V[fitidx,:].flatten())
    ax.plot(time[fitidx[0]:idx[-1] ],
            func(time[fitidx[0]:idx[-1]] - time[fitidx[0]],*popt), 'r') 
    # import ipdb; ipdb.set_trace()
    
    ax.axis('tight')
    remove_spines(ax)
    ax.set_ylabel('Voltage (mV)')
    ax.set_xlabel('Time (s)')

    return popt
    
def analyseSTEPprotocol(folder, ax = None):
    files, kfiles = gatherH5files(folder)
    Ke =  np.loadtxt(kfiles[0]) / 1e9
    V =  []
    I =  []
    spks = []
    ktrials =  [ os.path.basename(k).split('_')[0] for k in kfiles]
    print('Analysing step protocol...')
    spks = []
    for f in files:
        ent, info =  lcg.loadH5Trace(f)
        if not os.path.basename(f).split('.')[0] in ktrials:
            tmpV, tmpI, meta = compensateVoltage(ent, Ke, ['AnalogInput', 'Waveform'])
            V.append(tmpV)
            I.append(tmpI)
            spks.append(argfindspks(tmpV,-20))
    time = np.linspace( 0, info['tend'], len(tmpV))
    prot_time = np.cumsum(meta[::, 0])
    idx =  np.where((time > prot_time[0] - 0.1) & (time < prot_time[1] + .1))[0]
    V =  np.vstack(V).T
    I =  np.vstack(I).T
    ax.plot(time[idx], V[idx,], color = 'gray', lw = 0.6)
    ax.plot(time[idx], V[idx,0], color = 'k', lw = 1)
    spks =  [time[sp] for sp in spks]
    isi =  [np.diff(sp) for sp in spks]
    adapt_coeff =  [(i[-1] - i[0]) / i[0] for i in isi]
    print adapt_coeff
    ax.axis('tight')
    remove_spines(ax)
    ax.set_ylabel('Voltage (mV)')
    ax.set_xlabel('Time (s)')

    return np.mean(adapt_coeff)
    
def analyseRAMPprotocol(folder, ax = None):
    files, kfiles = gatherH5files(folder)
    Ke =  np.loadtxt(kfiles[0]) / 1e9
    V =  []
    I =  []
    spks = []
    ktrials =  [ os.path.basename(k).split('_')[0] for k in kfiles]
    print('Analysing ramp protocol...')
    for f in files:
        ent, info =  lcg.loadH5Trace(f)
        if not os.path.basename(f).split('.')[0] in ktrials:
            tmpV, tmpI, meta = compensateVoltage(ent, Ke, ['AnalogInput', 'Waveform'])
            V.append(tmpV)
            I.append(tmpI)
            spks.append(argfindspks(tmpV,-20))
    time = np.linspace( 0, info['tend'], len(tmpV))
    prot_time = np.cumsum(meta[::, 0])
    idx =  np.where(time > prot_time[-3])[0]
    V =  np.vstack(V).T
    I =  np.vstack(I).T
    ax[0].plot(time[idx], V[idx, 0], 'k')
    
    IFR_time =  []
    IFR = []
    threshold_current = [I[ii[0], jj] for jj, ii in enumerate(spks)]
    
    for sp in spks:
        sp =  time[sp]
        IFR_time.append(sp[1:] - np.diff(sp) / 2)
        IFR.append(1./ np.diff(sp))
    ax[1].plot(np.hstack(IFR_time), np.hstack(IFR), 'r-o',
                markerfacecolor = 'gray', markeredgecolor = 'k',
                markersize = 2)
    for a in ax: 
        a.axis('tight')
    tmp = ax[0].get_ylim()
    ax[1].set_xlim(ax[0].get_xlim())
    ax[1].set_ylim([0, max(ax[1].get_ylim()) * 1.2])
    ax[0].set_ylim(tmp)
    remove_spines(ax[0])
    ax[0].set_ylabel('Voltage (mV)')
    ax[0].set_xlabel('Time (s)')
    
    remove_spines(ax[1], v = 'right')
    ax[1].spines['top'].set_visible(False)
    ax[1].xaxis.set_visible(False)
    ax[1].spines['right'].set_color('red')
    ax[1].tick_params(axis='y', colors='red')
    ax[1].set_ylabel('Firing Freq (Hz)', color = 'red')
    return np.mean(threshold_current)

def remove_spines(ax, h = 'bottom', v = 'left'):
    nv = 'right'
    nh = 'top'
    if h in ['top']:
        nh =  'bottom'
        ax.get_xaxis().tick_top()
    else:
        ax.get_xaxis().tick_bottom()

    if  v in ['right']:
        nh =  'left'
        ax.get_yaxis().tick_right()
    else: 
        ax.get_yaxis().tick_left()
        
    ax.spines[nv].set_visible(False)
    ax.spines[nh].set_visible(False)
    ax.spines[v].set_visible(True)
    ax.spines[h].set_visible(True)

    ax.yaxis.set_label_position(v)
    ax.xaxis.set_label_position(h)
    ax.xaxis.set_ticks_position(h)
    ax.yaxis.set_ticks_position(v)
    
def create_figure():
    fig = plt.figure(figsize = (7, 7), facecolor = 'w',
                 edgecolor = None)
    ax = []
    ax.append(fig.add_axes([.05, .6, .45, .3]))
    ax.append(fig.add_axes([.5, .6, .45, .1]))
    ax.append(fig.add_axes([.5, .72, .45, .2]))
    ax.append(fig.add_axes([.1, .37, .8, .2]))
    newax = plt.axes([.1, .37, .8, .2], axisbg='none')
    ax.append(fig.add_axes(newax))
    ax.append(fig.add_axes([.1, .1, .3, .2]))
    ax.append(fig.add_axes([.5, .1, .4, .2]))
    fig.text(0.05,0.92,'A',fontsize=10,verticalalignment='bottom',
            horizontalalignment='right')
    fig.text(0.45,0.92,'B',fontsize=10,verticalalignment='bottom',
            horizontalalignment='right')
    fig.text(0.05,0.58,'C',fontsize=10,verticalalignment='bottom',
            horizontalalignment='right')
    fig.text(0.05,0.3,'D',fontsize=10,verticalalignment='bottom',
            horizontalalignment='right')
    fig.text(0.45,0.3,'E',fontsize=10,verticalalignment='bottom',
            horizontalalignment='right')
    return (fig, ax)

def analyze(directory='.'):
    experiments =  os.listdir(directory)
    for exp in experiments:
        if os.path.isdir(exp):
            (fig, ax) =  create_figure()
            threshold = analyseAPprotocol('{0}/ap/01'.format(exp), ax = ax[0])
            analyseVIprotocol('{0}/vi/01'.format(exp), ax[1:3])
            analyseRAMPprotocol('{0}/ramp/01'.format(exp), ax[3: 5])
            analyseTAUprotocol('{0}/tau/01'.format(exp), ax[5])
            analyseSTEPprotocol('{0}/steps/01'.format(exp), ax[6])
            for a in ax[:3]:
                a.axis('off')

            args = {}
            args['format'] = 'pdf'
            figname =  '{0}/ecode{0}.'.format(exp)
            fig.savefig('{0}{1}'.format(figname, args['format']),**args)
            args['format'] = 'png'
            fig.savefig('{0}{1}'.format(figname, args['format']),**args)
            plt.show()
