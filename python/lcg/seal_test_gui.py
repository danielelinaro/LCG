#! /usr/bin/env python

import sys
import os
import lcg
import numpy as np
import subprocess as sub
from PyQt4 import QtGui,QtCore
from matplotlib.backends.backend_qt4agg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.backends.backend_qt4agg import NavigationToolbar2QTAgg as NavigationToolbar
import matplotlib.pyplot as plt
plt.rc('font',**{'size':10})
plt.rc('axes',linewidth=0.8)
plt.rc('ytick',direction='out')
plt.rc('xtick',direction='out')

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

class Window(QtGui.QDialog):
    def __init__(self,parent = None):
        QtGui.QDialog.__init__(self,parent)
        self.fig = plt.figure()
        
        self.b_reset = QtGui.QPushButton('Reset')
        self.b_reset.clicked.connect(self.reset)

        self.layout = QtGui.QVBoxLayout()

        self.init_canvas()
        self.layout.addWidget(self.b_reset)
        self.setLayout(self.layout)
        self.init_lcg(0,0)
        # Init timers
        self.timer = QtCore.QTimer()
        self.plot_timer = QtCore.QTimer()
        QtCore.QObject.connect(self.timer,QtCore.SIGNAL('timeout()'),self.run_pulse)
        QtCore.QObject.connect(self.timer,QtCore.SIGNAL('timeout()'),self.plot)
        self.timer.start(20)
        self.plot_timer.start(30)                       
        
    def init_lcg(self,inchan=0,outchan=0,duration=15,
                 amplitude=10,sampling_rate=float(os.environ['SAMPLING_RATE'])):
        os.chdir('/tmp')
        self.filename = '/tmp/seal_test.h5'
        self.cfg_file = '/tmp/seal_test.xml'
        self.duration = duration
        duration = duration*1e-3
        self.stim_file = '/tmp/seal_test.stim'
        self.amplitude = amplitude #mV
        self.sampling_rate = sampling_rate
        channels = [{'type':'input',
                     'channel':inchan,
                     'factor':float(os.environ['AO_CONVERSION_FACTOR_VC']),
                     'units':os.environ['AO_UNITS_VC']}]
        
        channels.append({'type':'output',
                         'channel':outchan,
                         'factor':float(os.environ['AO_CONVERSION_FACTOR_VC']),
                         'units':os.environ['AO_UNITS_VC'],
                         'stimfile':self.stim_file})
        sys.argv = ['lcg-stimgen','-o',self.stim_file,
                    'dc', '-d',str(duration/4.0),'--','0',
                    'dc', '-d',str(duration),'--',str(amplitude),
                    'dc', '-d',str(duration/2.0),'--','0']
        lcg.stimgen.main)
        lcg.writeIOConfigurationFile(self.cfg_file,sampling_rate,
                                     duration*(7/4.0),channels,
                                     realtime=True,output_filename=self.filename)
            
        self.reset()

    def reset(self):
        self.time = None
        self.V = None
        self.I = []
        self.meanI = []
        self.resistance = []
        self.resistance_time = []
        self.count = 0
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
        self.layout.addWidget(self.toolbar)
        self.layout.addWidget(self.canvas)
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
        for ii in range(30):
            tmp, = self.ax[0].plot([],[],color = 'k',lw=0.5)
            self.raw_data_plot.append(tmp)
        self.mean_I_plot, = self.ax[0].plot([],[],c='r',lw=1)
        self.resistance_plot, = self.ax[1].plot([],[],color = 'r',lw=1)

        self.fig.show()

    def run_pulse(self):
        duration = self.duration
        sub.call('lcg-experiment -c {0} -V 4'.format(self.cfg_file),shell=True)
        try:
            ent,info = lcg.loadH5Trace(self.filename)
        except:
            print('File {0} not found.\n',)
            return
        for e in ent:
            if e['name'] in 'AnalogInput':
                self.I.append(e['data'])
                break
        if self.V is None:
            for e in ent:
                if e['name'] in 'Waveform':
                    self.V = e['data']
                    self.time = np.linspace(0,info['tend']*1.0e3,len(self.V))
                    break
        if len(self.I) > 30:
            del(self.I[0])
        self.count += 1
        allI = np.vstack(self.I).T
        idxpre = np.where(self.time<(-0.1+duration/4))[0]
        idxpost = np.where((self.time>(-1+(duration*5)/4)) & (self.time<(-0.1+(duration*5)/4)))[0]
        self.Iaxis_limits = [np.min(allI),np.max(allI)]

        if len(self.I) > 10:
            self.meanI = np.mean(allI[:,-8:],axis=1)
            Vpre = np.mean(self.V[idxpre])
            Vpost = np.mean(self.V[idxpost])
            Ipre = np.mean(self.meanI[idxpre])
            Ipost = np.mean(self.meanI[idxpost])
        else:
            self.meanI = self.I[-1]
            Vpre = np.mean(self.V[idxpre])
            Vpost = np.mean(self.V[idxpost])
            Ipre = np.mean(self.I[-1][idxpre])
            Ipost = np.mean(self.I[-1][idxpost])
        
        self.resistance_time.append(info['startTimeSec'])
        self.resistance.append(1e-3*((Vpost-Vpre)/(Ipost-Ipre)))
        sys.stdout.write('\rResistance is: {0:.0f}MOhm.'.format(self.resistance[-1]))


        return
    def plot(self):
        sys.stdout.flush()
        for ii in range(len(self.I)):
            if not len(self.raw_data_plot[ii].get_xdata()):
                self.raw_data_plot[ii].set_xdata(self.time)
            self.raw_data_plot[ii].set_ydata(self.I[ii])
            if ii > 10:
                break
        if len(self.I) > 1:
            self.mean_I_plot.set_xdata(self.time)
            self.mean_I_plot.set_ydata(self.meanI)
            self.resistance_plot.set_xdata(np.array(self.resistance_time - self.resistance_time[0])/60.0)
            self.resistance_plot.set_ydata(np.array(self.resistance))
            self.ax[0].set_xlim([self.time[0],self.time[-1]])
            self.ax[1].set_xlim([-1,0]+np.max(self.resistance_plot.get_xdata())+0.1)
            self.ax[1].set_ylim([0,np.max(np.array(self.resistance[-30:]))+10])
            self.ax[0].set_xlim([self.time[0],self.time[-1]])
            self.ax[0].set_ylim(self.Iaxis_limits)

        self.canvas.draw()

def main():
    app = QtGui.QApplication(sys.argv)
    widget = Window()#QtGui.QWidget()
    widget.resize(600,500)
    widget.setWindowTitle("LCG Seal Test")
    widget.show()

    sys.exit(app.exec_())


if __name__ == "__main__":
    main()
