#!/usr/bin/env python
import os
import sys
import getopt
import lcg
import aec
import subprocess as sub
import numpy as np
import glob
import matplotlib
from matplotlib import pyplot as plt

matplotlib.rc('axes',edgecolor='k',labelcolor='k')
matplotlib.rc('ytick',color='k')
matplotlib.rc('xtick',color='k')

description='''
Uses MatplotLib to plot a trace from an h5 file.
TODO: description of switches...
Usage: 
lcg-plot -f all
lcg-plot -f <filename> -k <kernel.dat>
lcg-plot -f last,2 # Plots the last 2 files
lcg-plot -f <filename> -t <entity to trigger on> --tpre 10 -- tpost 20
'''
usage='{0}\n'.format(description)

default_options={'filename':[],
                 'downsample':1,
                 'kernel':[],
                 'stack':True,
                 'trigger_entity':None,
                 'trigger_pre':5,
                 'trigger_post':10,
                 'average':False,
                 'trigger_thresh':50}

cc = np.array([[230,  20,  36],
               [19, 124,  56],
               [21,  69, 152],
               [239, 104,  28],
               [82,  24, 126],
               [143,  14,  25],
               [163,  31, 129],
               [96,  96,  96],
               [234,  66,  78],
               [196, 186,  87],
               [74, 136, 255],
               [247, 150,  73],
               [140,  79, 155],
               [140,  79, 155],
               [193,  90,  71],
               [204, 103, 165],
               [165, 165, 165]]*30)/255.0;

def parse_options():
    opt = default_options.copy()
    try:
        opts,args = getopt.getopt(sys.argv[1:], 'hf:d:k:t:',['help','average','threshold=','tpre=','tpost='])
    except:
        print(usage)
        sys.exit(1)
    for o,a in opts:
        if o in ['help','-h','--help']:
            print usage
            sys.exit(1)
        if o in ['-f']:
            opt['filename'] = [b for b in a.split(',')]
            filenames = np.sort(glob.glob('./*.h5'))
            kernel_files = []
            for f in glob.glob('./*_kernel.dat'):
                kernel_files.append(f[:-11])
            if 'all' in opt['filename']:
                opt['filename'] = []
                for f in filenames:
                    if not os.path.splitext(f)[0] in kernel_files:
                        opt['filename'].append(f)
            elif 'last' in opt['filename']:
                if len(opt['filename']) == 1:
                    last=1
                elif len(opt['filename']) == 2:
                    last = int(opt['filename'][1])
                else:
                    print('Option last takes only 1 input (number of files to plot).')
                    sys.exit(1)
                opt['filename'] = []
                if len(filenames) < last:
                    print('Not enough files.')
                    sys.exit(1)
                for f in filenames[-last:]:
                    if not os.path.splitext(f)[0] in kernel_files:
                        opt['filename'].append(f)
        if o in ['-k']:
            opt['kernel'] = [b for b in a.split(',')]
        if o in ['-d']:
            opt['downsample'] = float(a)
        if o in ['-t']:
            opt['trigger_entity'] = int(a)
        if o in ['--tpre']:
            opt['trigger_pre'] = float(a)
        if o in ['--tpost']:
            opt['trigger_post'] = float(a)
        if o in ['--threshold']:
            opt['trigger_thresh'] = float(a)
        if o in ['--average']:
            opt['average'] = True
    return opt

def plot_entities(ax,time,ent,offset=0,points=None,downsample=2,color=[1,1,1]):
    if points is None:
        points = len(ent[0]['data'])
    for i,e in enumerate(ent):
        try:
            ax[i].plot(time[offset:points:downsample],
                       e['data'][offset:points:downsample],color=color)
            ax[i].set_ylabel(e['units'])
        except:
            print('Different number of entities in file.')

def plot_triggered_entities(ax,channel,time,ent,threshold=50,opts=None,color=[.5,.5,.5]):
    # Find and cleanup trigger elements (threshold in Units/ms)
    srate = 1.0/np.diff(time[:2])[0]
    tmp = np.diff(ent[channel]['data'])*(srate/1.0e3)
    idx = np.where(tmp>threshold)[0]    
    tmpidx = np.where(tmp<-threshold)[0]
    # Look at the first peak to see if we should go for
    # positive or negative transitions
    if idx[0]>tmpidx[0]:
        idx = tmpidx
    idx = idx[np.diff(idx)>5]
    
    # Gather them
    npre = opts['trigger_pre']/1.0e3 * srate
    npost = opts['trigger_post']/1.0e3 * srate
    N = npre+npost
    all_data = []
    if len(idx)>0:
        time_vector = np.linspace(-opts['trigger_pre'],opts['trigger_post'],N)
        for i,e in enumerate(ent):
            data = np.zeros((N,len(idx)))
            for n in range(len(idx)):
                try:
                    data[:,n]=e['data'][idx[n]-npre:idx[n]+npost]
                except:
                    print("Failed one trigger...")
            if not color is None:
                ax[i].plot(time_vector,data,color=color)
            ax[i].set_ylabel(e['units'])
            all_data.append(data)
    return all_data

def compensateWithKernelOffline(ent,kernelFiles=[]):
    #TODO: Add check for compensation online!!!
#    names = np.array([e['name'] for e in ent])
    units = np.array([e['units'] for e in ent])
    findEntityByName = lambda entityName:np.where(
        names==entityName)[0]
    findEntityByUnit = lambda entityUnit:np.where(
        units==entityUnit)[0]
    if len(kernelFiles) > 0:
        # Active Electrode Compensation offline
        Iidx = findEntityByUnit('pA')
        Vidx = findEntityByUnit('mV')
        for ii,kfile in enumerate(kernelFiles):
            Ke = np.loadtxt(kfile)
            if ii >= len(Vidx):
                print(
                    '''Specified too many kernel files,
 contact developers please.''')
                break
            if len(Vidx) == 1: 
#Then there is only one channel, sum the currents
                I = np.sum([ent[jj]['data'] for jj in Iidx], axis=0)
            else:
                I = Iidx[Iidx[ii]]['data']
            V = ent[Vidx[ii]]['data']
            if not 'metadata' in ent[Vidx[ii]].keys():
                ent[Vidx[ii]]['data'] = aec.compensate(V, I, Ke*1e-9)

def plotAllEntitiesFromFile(fig, filename, kernelFiles = [],
                            filesCounter=[], ax = [],
                            downsampleFactor = 1):
    # TODO: Cleanup function (filesCounter and return values)
    ent, info = lcg.loadH5Trace(filename)
    # print('Loaded file %s'%filename)
    time = np.linspace(0,info['tend'],len(ent[0]['data']))
    compensateWithKernelOffline(ent,kernelFiles)
    if (not len(ax) == len(ent)) and len(ax):
        print('Trying to plot different experiments, cleaning up...')
        ax = []
        fig.clf()
        filesCounter = []
    if len(ax) == 0:
        ax.append(fig.add_subplot(len(ent),1,1, axisbg='w'))
        for i in range(1,len(ent)):
            ax.append(fig.add_subplot(len(ent),1,i+1, 
                                      sharex=ax[0], 
                                      axisbg='w'))
        [t.set_color('black') for t in ax[-1].xaxis.get_ticklines()]
        [t.set_color('black') for t in ax[-1].xaxis.get_ticklabels()]
        ax[-1].set_xlabel('Time (s)')
        for a in ax:
            a.grid(True,color=[0.7,0.7,0.7])
            a.set_axisbelow(True)
            a.spines['top'].set_visible(False)
            a.spines['right'].set_visible(False)
            a.yaxis.set_ticks_position('left')
            a.xaxis.set_ticks_position('bottom')
        for a in ax[:-1]:
            plt.setp(a.get_xticklabels(), visible=False)
    plot_entities(ax, time, ent, 0 , None, downsampleFactor,
                  color=cc[len(filesCounter),:])
    # Aesthetics related
    ax[0].set_xlim([time[0],time[-1]])
    filesCounter.append(filename)
    return ax,filesCounter

def main():
    opts = parse_options()

    fig = plt.figure(facecolor='white')
    ax = []
    counter = []
    for filename in opts['filename']:
        if os.path.isfile(filename):
            ax,counter = plotAllEntitiesFromFile(fig, filename, 
                                                 kernelFiles = opts['kernel'],
                                                 filesCounter=counter, ax = ax,
                                                 downsampleFactor = opts['downsample'])
    plt.show()


'''        if opts['trigger_entity'] is None:
            else:
                ccc = None
                if not opts['average']:
                    ccc = [.7,.7,.7]
                else:
                    print("Warning: Plotting average traces only!")
                data = plot_triggered_entities(ax,opts['trigger_entity'],time,ent,
                                               opts['trigger_thresh'],opts,color=ccc)
                # Plot average
                for j,d in enumerate(data):
                    time = np.linspace(-opts['trigger_pre'],opts['trigger_post'],np.shape(d)[0])
                    ax[j].plot(time,np.mean(d,1),color=cc[k,:])
    else:
        ax[-1].set_xlabel('Time from trigger (ms)')
        ax[0].set_xlim([time[0],time[-1]])
'''

if __name__ in ['__main__']:
    main()
