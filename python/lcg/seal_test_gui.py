#! /usr/bin/env python

import sys
import os
import lcg
import numpy as np
import subprocess as sub
import getopt
from PyQt4 import QtGui,QtCore
from matplotlib.backends.backend_qt4agg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.backends.backend_qt4agg import NavigationToolbar2QTAgg as NavigationToolbar
import matplotlib.pyplot as plt
import time
from scipy.io import savemat
import socket
from time import sleep
import atexit


plt.rc('font',**{'size':10})
plt.rc('axes',linewidth=0.8)
plt.rc('ytick',direction='out')
plt.rc('xtick',direction='out')
from scipy import optimize

description = '''
Measures the pipete or membrane resistance in 
voltage clamp or current clamp mode.

    Note: this script does not take over the amplifier
so you have to assure that you are in the correct mode
when using it. 
Using this wrongly is a nice way to get cells fried...

    Turn of the "Resistance test" or other amplifier features 
when using this.

    Options:
        -a: Amplitude of the pulses (5mV)
        -d: Duration of the pulses (5ms)
        -i: Inter-pulse interval (ms)
        --CC: Current clamp mode.
        --reset: runs lcg-zero before quitting
        --kernel Computes the kernel for AEC
        --remote-blind-patch clamp (server:port,axes)
        --patch-opts [hunt depth, max depth, step size, hunt step size]

     Tip: You can use the + and - keys to increase/decrease the holding potential by 
10mV (VC) or 25 pA (CC).
'''

def smooth(x,beta):
    ''' hanning window smoothing'''
    # extending the data at beginning and at the end
    # to apply the window at the borders
    s = np.r_[x[beta-1:0:-1],x,x[-1:-beta:-1]]
    w = np.hanning(beta)
    y = np.convolve(w/w.sum(),s,mode='valid')
    return y[5:len(y)-5]

exp1 = lambda p,x: p[0]*(np.exp(-x/p[1]))  + p[2]
exp2 = lambda p,x: p[0]*(np.exp(-x/p[1])) + p[2]*(np.exp(-x/p[3])) + p[4]
errfunc = lambda p, x, y: exp2(p, x) - y
env = lambda x:os.environ[x]

class Window(QtGui.QDialog):
    def parse_options(self,opts):
        defaults = {'ai':env('AI_CHANNEL_VC'),
                    'ao':env('AO_CHANNEL_VC'),
                    'amp':10,
                    'duration':15,
                    'holding':0,
                    'mode':'VC',
                    'kernel':False,
                    'reset':False,
                    'remoteManipulator':None,
                    'patchOptions':[300., 1000., 10., 2.]}
        options = defaults.copy()
        for o,a in opts:
            if o == '-I':
                options['ai'] = int(a)
            elif o == '-O':
                options['ao'] = int(a)
            elif o == '-d':
                options['duration'] = float(a)
            elif o == '-a':
                options['amp'] = float(a)
            elif o == '-H':
                options['holding'] = float(a)                
            elif o == '--kernel':
                options['kernel'] = True
            elif o == '--reset':
                options['reset'] = True
            elif o == '--CC':
                options['ai'] = int(env('AI_CHANNEL_CC'))
                options['ao'] = int(env('AO_CHANNEL_CC'))
                options['amp'] = -100.
                options['duration'] = 40.

                print('CC mode!')
                options['mode'] = 'CC'
            elif o == '--remote-blind-patch':
                options['remoteManipulator'] = a
            elif o == '--patch-opts':
                # [hunt depth, max depth, step size, hunt-step size]
                options['patchOptions'] = [float(ii) for ii in a.split(',')]
                print options['patchOptions']
        if options['mode'] == 'VC':
            options['kernel'] = False
        return options

    def __init__(self,parent = None,opts=[]):
        self.opts = self.parse_options(opts)
        QtGui.QDialog.__init__(self,parent)
        self.fig = plt.figure()
        
        self.b_reset = QtGui.QPushButton('Reset')
        self.b_reset.clicked.connect(self.reset)
        self.b_zoom = QtGui.QCheckBox('Auto Zoom')
        self.b_zoom.clicked.connect(self.autoZoom)
        self.b_zoom.setChecked(True)
        self.b_save = QtGui.QPushButton('Save')
        self.b_save.clicked.connect(self.save)
        self.b_fit = QtGui.QCheckBox('Fit decay')
        self.b_fit.clicked.connect(self.tryFit)
        self.b_fit.setChecked(False)
        self.b_running = QtGui.QCheckBox('Running')
        self.b_running.setChecked(True)
        if self.opts['kernel']:
            self.b_kernel = QtGui.QPushButton('AEC')
            self.b_kernel.clicked.connect(self.runKernel)

        self.layout = QtGui.QGridLayout()
        
        self.init_canvas()
        layout = QtGui.QHBoxLayout()
        layout.addWidget(self.b_reset)
        layout.addWidget(self.b_zoom)
        layout.addWidget(self.b_fit)
        layout.addWidget(self.b_running)
        layout.addWidget(self.b_save)
        if self.opts['kernel']:
             layout.addWidget(self.b_kernel)


        self.layout.addItem(layout,2,0,1,2)
        if not self.opts['remoteManipulator'] is None:
            self.init_remote_blind_patch()
            self.layout.addItem(self.rBlindPatch['layout'],1,2,1,1)
            print('Selected remote blind patch clamping mode.')

        self.setLayout(self.layout)
        self.init_lcg()
        self.reset()

        # Init timers
        self.timer = QtCore.QTimer()
        self.plot_timer = QtCore.QTimer()
        QtCore.QObject.connect(self.timer,QtCore.SIGNAL('timeout()'),self.run_pulse)
        QtCore.QObject.connect(self.plot_timer,QtCore.SIGNAL('timeout()'),self.plot)
        self.timer.start(self.duration*1.2)
        self.plot_timer.start(self.duration*3)                       
        
    def init_lcg(self):
        self.cwd = os.getcwd()
        os.chdir('/tmp')
        self.filename = '/tmp/seal_test.h5'
        self.cfg_file = '/tmp/seal_test.xml'
        self.duration = self.opts['duration']
        duration = self.duration*1e-3
        self.stim_file = '/tmp/seal_test.stim'
        self.amplitude = self.opts['amp'] #mV
        self.sampling_rate = env('SAMPLING_RATE')
        holding = self.opts['holding']
        channels = [{'type':'input',
                     'channel':self.opts['ai'],
                     'factor':float(os.environ['AI_CONVERSION_FACTOR_'+self.opts['mode']]),
                     'units':os.environ['AI_UNITS_'+self.opts['mode']]}]
        
        channels.append({'type':'output',
                         'channel':self.opts['ao'],
                         'factor':float(os.environ['AO_CONVERSION_FACTOR_'+self.opts['mode']]),
                         'units':os.environ['AO_UNITS_'+self.opts['mode']],
                         'resetOutput':False,
                         'stimfile':self.stim_file})

#        print('Conversion factors are {0}{2} and {1}{3}.'.format(
#                channels[0]['factor'],
#                channels[1]['factor'],
#                channels[0]['units'],
#                channels[1]['units']))
        
        sys.argv = ['lcg-stimgen','-o',self.stim_file,
                    'dc', '-d',str(duration/4.0),'--',str(holding),
                    'dc', '-d',str(duration),'--',str(self.amplitude+holding),
                    'dc', '-d',str(duration/2.0),'--',str(holding)]
        lcg.stimgen.main()
        lcg.writeIOConfigurationFile(self.cfg_file,self.sampling_rate,
                                     duration*(7/4.0),channels,
                                     realtime=True,
                                     output_filename=self.filename)
        atexit.register(self.on_exit)

    def on_exit(self):
        if self.opts['reset']:
            sub.call('lcg-zero')

    def keyPressEvent(self, event):
        step = 10
        holdStr = '{0} mV'
        if self.opts['mode'] == 'CC':
            step = 25
            holdStr = '{0} pA'
        if event.key() == 43:
            self.opts['holding'] += step
        elif event.key() == 45:
            self.opts['holding'] -= step
        self.init_lcg()
        self.holdingText.set_text(holdStr.format(self.opts['holding']))
        event.accept()


    def runKernel(self):
        # Pause 
        self.b_kernel.setEnabled(False)
        self.b_running.setChecked(False)
        sleep(0.5)
        xterm = 'xterm -fg white -bc -bg black -e '
        sub.call('cd {3} ; pwd ; {0} "lcg-kernel -d 1 -I {1} -O {2}"'.format(
                xterm, self.opts['ai'],self.opts['ao'],'/tmp'),shell=True)
        self.AEC = np.loadtxt('kernel.dat')
        self.b_kernel.setEnabled(True)
        self.b_running.setChecked(True)

    def tryFit(self):
        I = np.mean(np.vstack(self.I).T[:,-8:],axis=1)
        idxpost = np.where((self.time<(self.duration)))[0][-1]
        if self.opts['amp']>0:
            idxpre = np.argmax(I)
        else:
            idxpre = np.argmin(I)
        #V = np.vstack(self.V).T[:,-8:],axis=1)
        p0 = [10,0.4,0,0.001,0]
        p2,success = optimize.leastsq(errfunc, p0[:], args=(
                (self.time[idxpre:idxpost]-self.time[idxpre]),
                I[idxpre:idxpost]))
        maxtau = np.max(np.array(p2)[[1,3]])
        self.exp_param = p2
        self.fit_line.set_xdata(self.time[idxpre:idxpost])
        self.fit_line.set_ydata(exp2(p2,
                    self.time[idxpre:idxpost]-self.time[idxpre]))
#        print('Fit time constants are: {0:.2f} and {1:.2f}ms.'.format(p2[1],p2[3]))
#        print('Fit time constants are: {0}ms.'.format(p2[1]))
        self.tautext.set_text('{0:.2f}ms'.format(maxtau))
        
    def autoZoom(self):
        self.ax[0].set_ylim(self.Iaxis_limits)
        self.ax[0].set_xlim([self.time[0],self.time[-1]])
            
    def reset(self):
        self.time = []
        self.V = []
        self.I = []
        self.meanI = []
        self.resistance = []
        self.resistance_time = []
        self.exp_param = None
        self.count = 0
        self.autoscale = True
        self.fit_line.set_xdata([])
        self.fit_line.set_ydata([])
        self.fit_deduct.set_xdata([])
        self.fit_deduct.set_ydata([])

        sys.stdout.write('Starting...')
        sys.stdout.flush()

    def closeEvent(self,event):
        sys.stdout.write('\n Stopping timers...\n')
        self.timer.stop()
        self.plot_timer.stop()
        sys.stdout.flush()
        for f in [self.filename, self.cfg_file, self.stim_file]:
            os.unlink(f) 

    def init_canvas(self):
        self.canvas = FigureCanvas(self.fig)
        self.toolbar = NavigationToolbar(self.canvas,self)
        self.layout.addWidget(self.toolbar,0,0,1,2)
        self.layout.addWidget(self.canvas,1,0,1,2)
        # initialise axes and lines
        self.ax = []
        self.ax.append(self.fig.add_axes([0.11,0.4,0.84,0.58],
                                         axisbg='none',
                                         xlabel='time (ms)',
                                         ylabel='Current (pA)'))
        if self.opts['mode'] == 'CC':
            self.ax[0].set_ylabel('Voltage (mV)')
        if not self.opts['remoteManipulator']:
            self.ax.append(self.fig.add_axes([0.11,0.1,0.84,0.2],
                                             axisbg='none',
                                             xlabel='time (minutes)',
                                             ylabel='Resistance (Mohm)'))
        else:
            self.ax.append(self.fig.add_axes([0.11,0.1,0.75,0.2],
                                             axisbg='none',
                                             xlabel='time (minutes)',
                                             ylabel='Resistance (Mohm)'))
        for ax in self.ax:
            ax.spines['bottom'].set_color('black')
            ax.spines['top'].set_visible(False)
            ax.get_xaxis().tick_bottom()
            ax.get_yaxis().tick_left()
        self.raw_data_plot = []
        self.postI_line, = self.ax[0].plot([],[],'--',c='b',lw=1)
        self.preI_line, = self.ax[0].plot([],[],'--',c='b',lw=1)
        
        for ii in range(21):
            tmp, = self.ax[0].plot([],[],color = 'k',lw=0.5)
            self.raw_data_plot.append(tmp)
        self.fit_deduct, = self.ax[0].plot([],[],'-',c='c',lw=1.5)
        self.mean_I_plot, = self.ax[0].plot([],[],c='r',lw=1)
        self.resistance_plot, = self.ax[1].plot([],[],color = 'r',lw=1)
        self.autoscale = True
        self.Iaxis_limits = [-1500,1500]
        self.fit_line, = self.ax[0].plot([],[],'-',c='g',lw=1.5)
        self.Rtext = self.ax[0].annotate(
            "", xy=(0.99, 1), 
            xycoords='axes fraction',
            ha='right',va='top',color='r',fontsize=14)
        self.tautext = self.ax[0].annotate(
            "", xy=(0.99, 0.90), 
            xycoords='axes fraction',
            ha='right',va='top',color='g',fontsize=13)
        self.holdingText = self.ax[0].annotate(
            "", xy=(0.1, 0.1), 
            xycoords='axes fraction',
            ha='left',va='bottom',color='g',fontsize=12, fontweight='bold')

    def save(self):
        foldername = QtGui.QFileDialog.getExistingDirectory(
            self,
            'Select a folder to save',
            self.cwd)
        fname = time.strftime('%Y%m%d%H%M%S')
        filename = '{0}/sealtest_{1}.mat'.format(foldername,fname)
        print('\n Saving to folder {0}\n'.format(filename))
        self.tryFit()
        try:
            savemat('{0}'.format(filename),
                    {'timeR':self.resistance_time,
                     'R':self.resistance,
                     'V':np.vstack(self.V).T,
                     'I':np.vstack(self.I).T,
                     'expParam':self.exp_param},
                    do_compression=True,
                    oned_as='row')
        except:
            print('\nFile save failed.\n')

    def run_pulse(self):
        if self.b_running.isChecked():
            duration = self.duration
            sub.call('lcg-experiment -c {0} -V 4 --disable-replay'.format(self.cfg_file),shell=True)
            try:
                ent,info = lcg.loadH5Trace(self.filename)
            except:
                print('File {0} not found.\n',)
                return
            Idone = False
            Vdone = False
            for e in ent:
                if e['units'] in 'pA' and not Idone:
                    if not Idone:
                        self.I.append(e['data'])
                        Idone = True
                    else:
                        self.I[-1] = self.I[-1] + e['data']
                    break
            for e in ent:
                if e['units'] in 'mV':
                    if not Vdone:
                        self.V.append(e['data'])
                        self.time = np.linspace(0,info['tend']*1.0e3,
                                                len(self.V[-1]))
                        Vdone = True
                    else:
                        self.V[-1] = self.V[-1] + e['data']
                    break
            if len(self.I) > 20:
                del(self.I[0])
                del(self.V[0])
            self.count += 1
            allI = np.vstack(self.I).T
            allV = np.vstack(self.V).T
            idxpre = np.where(self.time<(-0.1+duration/4))[0]
            idxpost = np.where((self.time>(-1+(duration*5)/4)) & 
                               (self.time<(-0.1+(duration*5)/4)))[0]
            if len(self.I) > 11:
                self.meanI = np.mean(allI[:,-8:],axis=1)
                self.meanV = np.mean(allV[:,-8:],axis=1)
            else:
                self.meanI = self.I[-1]
                self.meanV = self.V[-1]
            self.Vpre = np.mean(self.meanV[idxpre])
            self.Vpost = np.mean(self.meanV[idxpost])
            self.Ipre = np.mean(self.meanI[idxpre])
            self.Ipost = np.mean(self.meanI[idxpost])
            self.resistance_time.append(info['startTimeSec'])
        #        print([(self.Vpost-self.Vpre),((self.Ipost-self.Ipre))])
            self.resistance.append(1e3*((self.Vpost-
                                         self.Vpre)/(self.Ipost-
                                                     self.Ipre)))
            if self.opts['mode'] == 'VC':
                self.Iaxis_limits = [np.min(allI),np.max(allI)]
            else:
                self.Iaxis_limits = [np.min(allV),np.max(allV)]


    def plot(self):
        sys.stdout.flush()
        if len(self.I) > 9:
            self.postI_line.set_xdata(np.array([self.time[0],self.time[-1]]))
            self.preI_line.set_xdata(np.array([self.time[0],self.time[-1]]))
            self.mean_I_plot.set_xdata(self.time)
            if self.opts['mode'] == 'CC':
                self.postI_line.set_ydata(self.Vpost*np.array([1,1]))
                self.preI_line.set_ydata(self.Vpre*np.array([1,1]))
                self.mean_I_plot.set_ydata(self.meanV)
                for ii in range(len(self.V)):
                    if not len(self.raw_data_plot[ii].get_xdata()):
                        self.raw_data_plot[ii].set_xdata(self.time)
                    self.raw_data_plot[ii].set_ydata(self.V[ii])
                    if ii > 10:
                        break
            else:
                self.postI_line.set_ydata(self.Ipost*np.array([1,1]))
                self.preI_line.set_ydata(self.Ipre*np.array([1,1]))
                self.mean_I_plot.set_ydata(self.meanI)
                for ii in range(len(self.I)):
                    if not len(self.raw_data_plot[ii].get_xdata()):
                        self.raw_data_plot[ii].set_xdata(self.time)
                    self.raw_data_plot[ii].set_ydata(self.I[ii])
                    if ii > 10:
                        break
            self.Rtext.set_text('{0:.1f}MOhm'.format(np.mean(self.resistance[-10:])))
            self.resistance_plot.set_xdata(np.array(self.resistance_time - self.resistance_time[0])/60.0)
            self.resistance_plot.set_ydata(np.array(self.resistance))
            self.ax[1].set_xlim([-1,0]+np.max(self.resistance_plot.get_xdata())+0.1)
            self.ax[1].set_ylim([0,np.max(np.array(self.resistance[-30:]))+10])
            if self.b_zoom.isChecked():
                self.autoZoom()
                self.autoscale = False
            if not self.exp_param is None:
                maxtau = np.max(np.array(self.exp_param)[[1,3]])
                I = np.mean(np.vstack(self.I).T[:,-8:],axis=1)
                idxpost = np.where((self.time<(self.duration)))[0][-1]
                if self.opts['amp']>0:
                    idxpre = np.argmax(I)
                else:
                    idxpre = np.argmin(I)
                self.fit_deduct.set_xdata(self.time[idxpre:idxpost])
                self.fit_deduct.set_ydata(
                    exp1([-(self.Ipost-self.Ipre),
                           maxtau,self.Ipre + (self.Ipost-self.Ipre)],
                         self.time[idxpre:idxpost]-self.time[idxpre]))
        self.canvas.draw()
    ### REMOTE BLIND PATCH ####
    def rBlindPatch_retract(self):
        print('Retracting Pipette')
        self.rBlindPatch['mode'] = 'retract'

        pass
    def rBlindPatch_resetResistance(self):
        print('Resetting pipette resistance')
        self.rBlindPatch['pipetteResistance'] = np.mean(np.array(self.resistance)[-10:]) 
        pass
    def rBlindPatch_zeroPosition(self):
        print('Zero-ing pipette position')
        try:
            self.rBlindPatch['socket'].sendall('zero')
        except:
            print('Zero command failed.')
        self.rBlindPatch['absPos'] = []
        self.rBlindPatch['position'] = []
        self.rBlindPatch['time'] = []
        self.ax[2].set_ylim([0,self.rBlindPatch['maxDepth']])
    def rBlindPatch_advance(self):
        print('Advancing pipette!')
        self.rBlindPatch['mode'] = 'advance'
    def rBlindPatch_stop(self):
        print('Stopping!!')
        self.rBlindPatch['mode'] = 'stop'
    def rBlindPatch_evolve(self):
        # ask position
        self.rBlindPatch['socket'].sendall('position')
        pos = self.rBlindPatch['socket'].recv(1024).split(',')
        self.rBlindPatch['absPos'].append([float(p[2:]) for p in pos])
        self.rBlindPatch['position'].append(np.sign(self.rBlindPatch['absPos'][-1][2])*np.sqrt(
                np.sum([p**2 for p in self.rBlindPatch['absPos'][-1]])))
        self.rBlindPatch['time'].append(time.time())
        self.rBlindPatch['positionPlot'].set_xdata(
            (np.array(self.rBlindPatch['time']) - self.resistance_time[0])/60.0)
        self.rBlindPatch['positionPlot'].set_ydata(np.array(self.rBlindPatch['position']))
        self.ax[2].set_xlim([-1,0]+np.max(self.rBlindPatch['positionPlot'].get_xdata())+0.1)
#        self.ax[2].set_ylim([0,np.max(np.array(self.rBlindPatch['position']))+50])
        self.rBlindPatch['patchResistancePlot'].set_xdata(np.array(self.ax[1].get_xlim()))
        self.rBlindPatch['patchResistancePlot'].set_ydata(np.array([1,1])*self.rBlindPatch['pipetteResistance'])
        if self.rBlindPatch['mode'] == 'hunt':
            self.rBlindPatch['modeText'].set_color('orange')
            # in hunting mode, advance small step
            self.rBlindPatch['socket'].sendall('move {0}={1}'.format(
                    self.rBlindPatch['axes'],self.rBlindPatch['huntStep']))
            if np.mean(np.array(self.resistance)[-5:]) > self.rBlindPatch['pipetteResistance']*1.4:
                print('Cell found??!?')
                self.rBlindPatch['mode'] = 'stop'
                self.rBlindPatch['modeText'].set_color('green')

        elif self.rBlindPatch['mode'] == 'advance':
            self.rBlindPatch['modeText'].set_color('yellow')
            # in mode advance make a large step until targetDepth
            self.rBlindPatch['socket'].sendall('move {0}={1}'.format(
                    self.rBlindPatch['axes'],self.rBlindPatch['step']))
            if ((self.rBlindPatch['position'][-1] + 
                 self.rBlindPatch['step']) > self.rBlindPatch['targetDepth']):
                self.rBlindPatch['mode'] = 'hunt'
        elif self.rBlindPatch['mode'] == 'retract':
            self.rBlindPatch['modeText'].set_color('blue')
            # in mode retract, go in the oposite direction
            if (self.rBlindPatch['position'][-1] < -50):
                self.rBlindPatch['mode'] = 'stop'
            if ((self.rBlindPatch['position'][-1] + 
                 self.rBlindPatch['step']) < self.rBlindPatch['targetDepth']):
                self.rBlindPatch['socket'].sendall('move {0}={1}'.format(
                        self.rBlindPatch['axes'],-self.rBlindPatch['step']*1.5))
            else:
                self.rBlindPatch['socket'].sendall('move {0}={1}'.format(
                        self.rBlindPatch['axes'],-self.rBlindPatch['huntStep']*2.))
        if (not self.rBlindPatch['mode'] == 'retract') and (
            (self.rBlindPatch['position'][-1] + self.rBlindPatch['step'])
            >= self.rBlindPatch['maxDepth']):
            self.rBlindPatch['mode'] = 'stop'
            
        self.rBlindPatch['modeText'].set_text(self.rBlindPatch['mode'])
        self.rBlindPatch['pText'].set_text(self.rBlindPatch['position'][-1])
        #print [self.rBlindPatch['mode'], self.rBlindPatch['position'][-1]]

    def init_remote_blind_patch(self):
        address = self.opts['remoteManipulator'].split(':')
        self.rBlindPatch = {}
        self.rBlindPatch['absPos'] = []
        self.rBlindPatch['position'] = []
        self.rBlindPatch['time'] = []
        self.rBlindPatch['mode'] = 'stop'
        self.rBlindPatch['pipetteResistance'] = np.nan
        # [hunt depth, max depth, step size, hunt-step size]
        self.rBlindPatch['axes'] = address[2]
        self.rBlindPatch['step'] = self.opts['patchOptions'][2]
        self.rBlindPatch['huntStep'] = self.opts['patchOptions'][3]
        self.rBlindPatch['targetDepth'] = self.opts['patchOptions'][0]
        self.rBlindPatch['maxDepth'] = self.opts['patchOptions'][1]
        print self.rBlindPatch['maxDepth'],self.rBlindPatch['targetDepth'] 
        self.rBlindPatch['buttons'] = {}
        self.rBlindPatch['buttons']['zeroPosition'] = QtGui.QPushButton('Zero Position')
        self.rBlindPatch['buttons']['zeroPosition'].clicked.connect(
            self.rBlindPatch_zeroPosition)
        self.rBlindPatch['buttons']['resetResistance'] = QtGui.QPushButton(
            'Reset Pipette Resistance')
        self.rBlindPatch['buttons']['resetResistance'].clicked.connect(
            self.rBlindPatch_resetResistance)
        self.rBlindPatch['buttons']['retract'] = QtGui.QPushButton('Retract')
        self.rBlindPatch['buttons']['retract'].clicked.connect(
            self.rBlindPatch_retract)
        self.rBlindPatch['buttons']['advance'] = QtGui.QPushButton('Start')
        self.rBlindPatch['buttons']['advance'].clicked.connect(
            self.rBlindPatch_advance)
        self.rBlindPatch['buttons']['stop'] = QtGui.QPushButton('Stop!')
        self.rBlindPatch['buttons']['stop'].clicked.connect(
            self.rBlindPatch_stop)
        layout = QtGui.QVBoxLayout()
        layout.addWidget(self.rBlindPatch['buttons']['zeroPosition'])
        layout.addWidget(self.rBlindPatch['buttons']['resetResistance'])
        layout.addWidget(self.rBlindPatch['buttons']['retract'])
        layout.addWidget(self.rBlindPatch['buttons']['advance'])
        layout.addWidget(self.rBlindPatch['buttons']['stop'])

        self.rBlindPatch['layout'] = layout 
        self.rBlindPatch['timerObj'] = QtCore.QTimer()
        QtCore.QObject.connect(self.rBlindPatch['timerObj'],
                               QtCore.SIGNAL('timeout()'),
                               self.rBlindPatch_evolve)
        self.rBlindPatch['socket'] = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        try:
            self.rBlindPatch['socket'].connect((address[0],int(address[1])))
            print('Connected to remote manipulator (maybe...)')
        except:
            print('Could not connect to: %s'%(self.opts['remoteManipulator']))
            sys.exit(1)
        self.ax.append(self.ax[1].twinx())
        ax = self.ax[-1]
        ax.spines['right'].set_visible(True)
        ax.get_yaxis().tick_right()
        ax.set_ylabel('Position',)
        self.rBlindPatch['positionPlot'], = ax.plot([],[],color = 'b',lw=1)
        ax.set_ylim([0,self.rBlindPatch['maxDepth']])
        self.rBlindPatch['patchResistancePlot'], = self.ax[1].plot([],[],
                                                                   linestyle='--',color = 'g',lw=1)
        self.rBlindPatch['modeText'] =  self.ax[1].annotate("",
                                                            xy=(0.1, 0.1), 
                                                            xycoords='axes fraction',
                                                            ha='left',va='bottom',
                                                            color='k',fontsize=12)
        self.rBlindPatch['pText'] =  self.ax[1].annotate("",
                                                         xy=(0.99, 1), 
                                                         xycoords='axes fraction',
                                                         ha='right',va='top',
                                                         color=[.1,.1,.9],fontsize=12)
        self.rBlindPatch['timerObj'].start(1000) # Starts with 1 second period
        
    ############################
def main():
    try:
        opts,args = getopt.getopt(sys.argv[1:], 'H:I:O:a:d:',
                                  ['kernel',
                                   'CC',
                                   'remote-blind-patch=',
                                   'reset',
                                   'patch-opts='])
    except getopt.GetoptError, err:
        print(err)
        print(description)
        sys.exit(1)

    app = QtGui.QApplication(sys.argv)
    widget = Window(opts=opts)
    widget.resize(750,500)
    widget.setWindowTitle("LCG Seal Test")
    widget.show()
    sys.exit(app.exec_())


if __name__ == "__main__":
    main()
