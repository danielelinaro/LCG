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
        - a: Amplitude of the pulses (5mV)
        - d: Duration of the pulses (5ms)
        - i: Inter-pulse interval (ms)
Press CTR-C to exit.
'''

def smooth(x,beta):
    ''' hanning window smoothing'''
    # extending the data at beginning and at the end
    # to apply the window at the borders
    s = np.r_[x[beta-1:0:-1],x,x[-1:-beta:-1]]
    w = np.hanning(beta)
    y = np.convolve(w/w.sum(),s,mode='valid')
    return y[5:len(y)-5]

#exp1 = lambda p,x: p[0]*(np.exp(-x/p[1]))  + p[2]
exp2 = lambda p,x: p[0]*(np.exp(-x/p[1])) + p[2]*(np.exp(-x/p[3])) + p[4]
errfunc = lambda p, x, y: exp2(p, x) - y
env = lambda x:os.environ[x]

class Window(QtGui.QDialog):
    def parse_options(self,opts):
        defaults = {'ai':env('AI_CHANNEL'),
                    'ao':env('AO_CHANNEL'),
                    'amp':10,
                    'duration':15,
                    'holding':0,
                    'mode':'VC',
                    'kernel':False}
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
            elif o == '--CC':
                options['mode'] = 'CC'
        return options

    def __init__(self,parent = None,opts=[]):
        self.opts = self.parse_options(opts)
        print self.opts
        print opts
        QtGui.QDialog.__init__(self,parent)
        self.fig = plt.figure()
        
        self.b_reset = QtGui.QPushButton('Reset')
        self.b_reset.clicked.connect(self.reset)
        self.b_zoom = QtGui.QPushButton('Auto Zoom')
        self.b_zoom.clicked.connect(self.autoZoom)
        self.b_save = QtGui.QPushButton('Save')
        self.b_save.clicked.connect(self.save)
        self.b_fit = QtGui.QPushButton('Fit decay')
        self.b_fit.clicked.connect(self.tryFit)

        self.layout = QtGui.QFormLayout()

        self.init_canvas()
        layout = QtGui.QHBoxLayout()
        layout.addWidget(self.b_reset)
        layout.addWidget(self.b_zoom)
        layout.addWidget(self.b_fit)
        layout.addWidget(self.b_save)


        self.layout.addRow(layout)

        self.setLayout(self.layout)
        self.init_lcg()
        # Init timers
        self.timer = QtCore.QTimer()
        self.plot_timer = QtCore.QTimer()
        QtCore.QObject.connect(self.timer,QtCore.SIGNAL('timeout()'),self.run_pulse)
        QtCore.QObject.connect(self.plot_timer,QtCore.SIGNAL('timeout()'),self.plot)
        self.timer.start(20)
        self.plot_timer.start(150)                       
        
    def init_lcg(self):
        
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

        print('Conversion factors are {0}{2} and {1}{3}.'.format(
                channels[0]['factor'],
                channels[1]['factor'],
                channels[0]['units'],
                channels[1]['units']))
        
        sys.argv = ['lcg-stimgen','-o',self.stim_file,
                    'dc', '-d',str(duration/4.0),'--',str(holding),
                    'dc', '-d',str(duration),'--',str(self.amplitude+holding),
                    'dc', '-d',str(duration/2.0),'--',str(holding)]
        lcg.stimgen.main()
        lcg.writeIOConfigurationFile(self.cfg_file,self.sampling_rate,
                                     duration*(7/4.0),channels,
                                     realtime=True,output_filename=self.filename)
            
        self.reset()

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
        self.fit_line.set_xdata(self.time[idxpre:idxpost])
        self.fit_line.set_ydata(exp2(p2,
                    self.time[idxpre:idxpost]-self.time[idxpre]))
        print('Fit time constants are: {0:.2f} and {1:.2f}ms.'.format(p2[1],p2[3]))
        self.exp_param = p2
#        print('Fit time constants are: {0}ms.'.format(p2[1]))
        
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
        self.count = 0
        self.autoscale = True
        self.fit_line.set_xdata([])
        self.fit_line.set_ydata([])

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
        self.layout.addRow(self.toolbar)
        self.layout.addRow(self.canvas)
        # initialise axes and lines
        self.ax = []
        self.ax.append(self.fig.add_axes([0.15,0.4,0.84,0.58],axisbg='none',xlabel='time (ms)',ylabel='Current (pA)'))
        self.ax.append(self.fig.add_axes([0.15,0.1,0.84,0.2],axisbg='none',xlabel='time (minutes)',ylabel='Resistance (Mohm)'))
        for ax in self.ax:
            ax.spines['bottom'].set_color('black')
            ax.spines['top'].set_visible(False)
            ax.spines['right'].set_visible(False)
            ax.get_xaxis().tick_bottom()
            ax.get_yaxis().tick_left()
        self.raw_data_plot = []
        self.postI_line, = self.ax[0].plot([],[],'--',c='b',lw=1)
        self.preI_line, = self.ax[0].plot([],[],'--',c='b',lw=1)
        
        for ii in range(30):
            tmp, = self.ax[0].plot([],[],color = 'k',lw=0.5)
            self.raw_data_plot.append(tmp)
        self.mean_I_plot, = self.ax[0].plot([],[],c='r',lw=1)
        self.resistance_plot, = self.ax[1].plot([],[],color = 'r',lw=1)
        self.autoscale = True
        self.Iaxis_limits = [-1500,1500]
        self.fit_line, = self.ax[0].plot([],[],'-',c='g',lw=1.5)

        self.fig.show()
    def save(self):
        foldername = QtGui.QFileDialog.getExistingDirectory(self, 'Select a folder to save',
                                                    env('HOME')+'/experiments')
        fname = time.strftime('%Y%m%d%H%M%S')
        filename = '{0}/seal_test_{1}.mat'.format(foldername,fname)
        print('\n Saving to folder {0}\n'.format(filename))
        self.tryFit()
        savemat('{0}'.format(filename),
                {'timeR':self.resistance_time,
                 'R':self.resistance,
                 'V':np.vstack(self.V).T,
                 'I':np.vstack(self.I).T,
                 'expParam':self.exp_param},
                do_compression=True,
                oned_as='row')
    def run_pulse(self):
        duration = self.duration
        sub.call('lcg-experiment -c {0} -V 4'.format(self.cfg_file),shell=True)
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
        self.Iaxis_limits = [np.min(allI),np.max(allI)]

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
        
        return
    def plot(self):
        sys.stdout.flush()
        for ii in range(len(self.I)):
            if not len(self.raw_data_plot[ii].get_xdata()):
                self.raw_data_plot[ii].set_xdata(self.time)
            self.raw_data_plot[ii].set_ydata(self.I[ii])
            if len(self.resistance)>10:
                sys.stdout.write('\rResistance is: {0:.1f}MOhm.'.format(np.mean(self.resistance[-10:])))

            if ii > 10:
                break
        if len(self.I) > 1:
            self.postI_line.set_xdata(np.array([self.time[0],self.time[-1]]))
            self.postI_line.set_ydata(self.Ipost*np.array([1,1]))
            self.preI_line.set_xdata(np.array([self.time[0],self.time[-1]]))
            self.preI_line.set_ydata(self.Ipre*np.array([1,1]))

            self.mean_I_plot.set_xdata(self.time)
            self.mean_I_plot.set_ydata(self.meanI)
            self.resistance_plot.set_xdata(np.array(self.resistance_time - self.resistance_time[0])/60.0)
            self.resistance_plot.set_ydata(np.array(self.resistance))
            self.ax[1].set_xlim([-1,0]+np.max(self.resistance_plot.get_xdata())+0.1)
            self.ax[1].set_ylim([0,np.max(np.array(self.resistance[-30:]))+10])
            if self.autoscale:
                self.autoZoom()
                self.autoscale = False
        self.canvas.draw()

def main():
    print sys.argv
    try:
        opts,args = getopt.getopt(sys.argv[1:], 'H:I:O:a:d:',['kernel','CC'])
    except getopt.GetoptError, err:
        print(err)
        print(usage)
        sys.exit(1)

    app = QtGui.QApplication(sys.argv)
    widget = Window(opts=opts)#QtGui.QWidget()
    widget.resize(600,500)
    widget.setWindowTitle("LCG Seal Test")
    widget.show()

    sys.exit(app.exec_())


if __name__ == "__main__":
    main()
