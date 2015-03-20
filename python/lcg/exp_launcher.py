#! /usr/bin/env python
from PyQt4 import QtGui,QtCore
from matplotlib.backends.backend_qt4agg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.backends.backend_qt4agg import NavigationToolbar2QTAgg as NavigationToolbar

#from ipdb import set_trace
import sys
import os
import subprocess as sub
import ConfigParser
import matplotlib.pylab as plt
from argparse import ArgumentParser
from getpass import getuser
import multiprocessing as mp
from time import gmtime, strftime
import atexit
from glob import glob
from functools import partial
import numpy as np
import re

import lcg
from lcg.plot_file import plotAllEntitiesFromFile
from lcg.create_folder import (defaultConfig,
                               expsection,
                               logsection,
                               createDefaultParameters,
                               createFoldersAndInfoFile,
                               makeIncrementingFolders)

protsection = 'protocol'
dummyNoisecommand = ' '.join(['lcg-stimgen ',
                              'dc -d 1 -- 0 ',
                              'ou -d {2} -- {3} {4} {5}',
                              ' dc -d 1 -- 1 |',
                              'lcg-stimulus -n {0} -i {1} --model'])
defaultProtocol = [
    {'name': 'Noise Protocol', 'command': dummyNoisecommand,
     'parameters': 'Number of trials,Inter-trial interval,Duration,Mean,Standard deviation,Time constant',
     'defaults': '10,1,3,100,150,20',
     'foldername': 'ou'},
    {'name': 'Steps Protocol',
     'command': 'lcg-steps -n {0} -i {1} -d {2} -a {3},{4},{5} {6}',
     'parameters': 'Number of trials,Inter-trial interval,Duration,Start amplitude,Max amplitude,Step increment,Additional options',
     'defaults': '1,5,1,-200,+250,50,--model',
     'foldername': 'steps'},
    ]
protocolKeys = ['name','command','parameters','defaults','foldername']

def createDefaultConfig(cfg,appendDefaultParameters=False,cfgfile = None):
    if cfg is None:
        cfg = ConfigParser.ConfigParser()
    if appendDefaultParameters:
        createDefaultParameters(cfg,False)
    for defaultProt in defaultProtocol:
        sec = defaultProt[protocolKeys[0]]
        cfg.add_section(sec)
        for k in protocolKeys:
            cfg.set(sec,k,defaultProt[k])
    if not cfgfile is None:
        with open(cfgfile,'w') as f:
            print('Writing dummy protocols to: {0}'.format(cfgfile))
            cfg.write(f)

def readProtocolSection(cfg,sec):
    if protsection in sec.lower():
        # Each protocol requires the fields: name,
        #command, parameters, defaults and foldername
        tmp = {'name':sec}
        for key in protocolKeys[1:]:
            if key in ['command','foldername']:
                tmp[key] = cfg.get(sec, key)
            else:
                tmp[key] = cfg.get(sec, key).split(',')
            # Guess datatype of parameters
        values =  []
        for val in tmp['defaults']:
            try:
                values.append(int(val))
            except ValueError:
                try:
                    values.append(float(val))
                except ValueError:
                    values.append(val)
        tmp['defaults'] =  values
        tmp['values'] =  values
        # Confirm dimensions of defaults and parameters
        nparameters = len(np.unique(re.compile('{*.}').findall(tmp['command'])))
        if (len(tmp['values'])
            == len(tmp['parameters'])
            == nparameters):
            return tmp.copy()
        else:
            print('''
Wrong number of parameters in protocol {0}.
Check the number of curly brackets in the command,
the number of parameters, and the number of defaults!'''.format(tmp['name']))
            raise
    return None

def parseCfg(cfg, cfgfile):
    '''Parses the configuration files
    A configuration file is just an ini file with 
each section representing the name of a protocol. 
Allowed fields are:
    - command : the command to be executed
    - parameters : the parameter names to be substituted
    - defaults : the default values to the parameters
    - foldername : the name of the main folder
    '''
    protocols =  []
    general = {}
    log = {}
    logkeys = []
    for sec in cfg.sections():
        print('Reading section: {0}'.format(sec))
        tmp = readProtocolSection(cfg,sec)        
        if not tmp is None:
            protocols.append(tmp.copy())
        elif sec == expsection:
            for p,v in cfg.items(expsection):
                general[p] = v
        elif sec == logsection:
            for p,v in cfg.items(logsection):
                log[p] = v
    if not len(protocols):
        createDefaultConfig(cfg,
                            (len(general) < 1),
                            cfgfile)
        protocols.extend(defaultProtocol)
    # Update subfolders
    general['subfolders'] = updateSubfolders(protocols)
    return protocols,general,log

def updateSubfolders(protocols):
    #Fix the subfolders...
    if len(protocols):
        subfolders = []
        for p in protocols:
            subfolders.append(p['foldername'])
        subfolders = ','.join(subfolders)
    else:
        subfolders = ''
    return subfolders

def which(program):
    def is_exe(fpath):
        return os.path.isfile(fpath) and os.access(fpath, os.X_OK)
    
    fpath, fname = os.path.split(program)
    if fpath:
        if is_exe(program):
            return program
    else:
        for path in os.environ["PATH"].split(os.pathsep):
            path = path.strip('"')
            exe_file = os.path.join(path, program)
            if is_exe(exe_file):
                return exe_file

    return None


externalProcess = None
externalProcessLabel = None
runStatus = 'Idle'
printToStdOut = lambda x:sys.stdout.write(x);sys.stdout.flush()
experimentFolder = None

class RunButton(QtGui.QPushButton):
    def __init__(self, command='', par=[], 
                 name = '',folder = '', 
                 holdTerminal = None, useLastFolder = None, 
                 *args, **kwargs):
        super(RunButton,self).__init__(*args, **kwargs)
        self.command = command
        self.par = par
        self.name = name
        self.timer = QtCore.QTimer()
        self.alternateText = False
        self.holdTerminal = holdTerminal
        self.useLastFolder = useLastFolder
        self.lastFolder = None
        QtCore.QObject.connect(self.timer,
                               QtCore.SIGNAL('timeout()'),
                               self.handleTimer)
        self.app = 'xterm'
        if which(self.app) is None:
            print('Install xterm (sudo apt-get install xterm).')
            sys.exit(1)
        self.folder = folder

    def runCommand(self):
        global externalProcess
        global experimentFolder
        global externalProcessLabel
        if experimentFolder is None and not (self.folder is None):
            externalProcessLabel.setText('Experiment folder not set.')        
            externalProcessLabel.setStyleSheet(
                'QLabel {color:red; font-weight:bold; font-size:16}')
            return
        if not externalProcess is None:
            externalProcessLabel.setText('Another process is running.')
            return
        if not self.folder is None:
            folderpath ='{0}/{1}'.format(experimentFolder,self.folder) 
            if not os.path.isdir(folderpath):
                os.makedirs(folderpath)
            os.chdir(folderpath)
            foundFolder = False
            for root,dirnames,filenames in os.walk(os.getcwd()):
                for subfolder in dirnames:
                    tmpFolder = '{0}'.format(subfolder)
                    h5files = glob('{0}/*.h5'.format(tmpFolder))
                    if not len(h5files):
                        foundFolder = True
                        runFolder = tmpFolder
                break
            if ((not foundFolder and 
                 not self.useLastFolder.isChecked())
                or (self.useLastFolder.isChecked() and 
                    self.lastFolder is None)):
                runFolder = makeIncrementingFolders(
                    folderPattern='[01]',
                    dryRun=False)
            elif self.useLastFolder.isChecked():
                runFolder = self.lastFolder
            print [self.lastFolder,runFolder,self.useLastFolder.isChecked(),runFolder]
            #if not foundFolder:
            #    runFolder = makeIncrementingFolders(
            #        folderPattern='[01]',
            #        dryRun=False)
            os.chdir(runFolder)
            self.lastFolder = os.getcwd()

        par = []
        for p in self.par:
            par.append(p.text())
        options = []
        if not self.holdTerminal is None and self.holdTerminal.isChecked():
            options.append('-hold -im')
        if len(par):
            command = '{0} {1} -e "{2}'.format(self.app,
                                                ' '.join(options),
                                                self.command.format(*par))
                                     
        else:
            command = '{0} {1} -e "{2}'.format(self.app,
                                                ' '.join(options),
                                                self.command)
        
        if not self.holdTerminal is None and self.holdTerminal.isChecked():
            command += ' ; exec /bin/bash"'
        else:
            command += '"'
        print command
        
        externalProcess = sub.Popen(command, shell=True)
        externalProcessLabel.setStyleSheet(
            'QLabel {color:red; font-weight:bold; font-size:16}')

        runStatus = '{0}.'.format(self.name)
        self.timer.start(300)
        return

    def handleTimer(self):
        self.alternateText = not self.alternateText
        global externalProcess
        global externalProcessLabel
        currentTime = strftime('%H:%M:%S',gmtime())
        if externalProcess is None:
            print('externalProcess is None...')
            self.timer.stop()
        elif not externalProcess.poll() is None:
            externalProcess = None
            runStatus = 'Idle'
            completedString = '{0} - Completed {1}.'.format(
                currentTime,self.name)
            if hasattr(self,'lastRunFolder'):
                completedString = completedString + '\nData in: {0}'.format(
                    self.lastRunFolder)
            externalProcessLabel.setText(completedString)
            externalProcessLabel.setStyleSheet(
                'QLabel {color:green; font-weight:bold; font-size:16}')
            print(completedString)
            self.timer.stop()
        else:
            if self.alternateText:
                externalProcessLabel.setText('{0} - Busy with {1}'.
                                             format(currentTime,self.name))
            else:
                externalProcessLabel.setText(' ')
                
@atexit.register
def terminate():
    global externalProcess
    if not externalProcess is None:
        externalProcess.communicate()
        # Not sure this is needed...

class LCG_COMMANDER(QtGui.QDialog):
    def __init__(self,cfgfile=''):
        super(LCG_COMMANDER,self).__init__()
        config = ConfigParser.ConfigParser()
        config.read(cfgfile)
        (self.protocols,
         self.generalParameters,
         self.logParameters) = parseCfg(config,cfgfile)
        config.read(cfgfile)
        self.config = config
        printToStdOut('Initializing window')
        self.windowMain = QtGui.QHBoxLayout()
        self.windowTabs = QtGui.QTabWidget()
        self.initProtocols(),printToStdOut('.')
        self.initPlotting(),printToStdOut('.')
        self.initExperiment(),printToStdOut('.')
        self.windowTabs.addTab(self.expTab,'General parameters')
        self.windowTabs.addTab(self.protTab,'Protocols')
        self.windowTabs.addTab(self.plotTab,'Data display')
        self.windowMain.addWidget(self.windowTabs)
        self.setLayout(self.windowMain)
        screen = QtGui.QDesktopWidget().screenGeometry()
        self.setGeometry(screen.width()*0.05,
                         screen.height()*0.05,
                         screen.width()*0.7,
                         screen.height()*0.7)
        self.setWindowTitle('LGC Experiment Manager')
        self.show(),printToStdOut(' Done.\n\n')

    def initExperiment(self):
        self.parentFolder = os.path.abspath(os.getcwd())
        self.expBox = []
        self.expTab = QtGui.QWidget()
        param = self.generalParameters
        self.expParameterWidgets = []
        groups = []
        groups.append(QtGui.QGroupBox('Folder parameters'))
        self.expBox.append(QtGui.QFormLayout())
        self.parentFolderLabel = QtGui.QLabel(
            'Parent folder: {0}'.format(
                os.path.dirname(self.parentFolder)))
        self.expBox[-1].addRow(self.parentFolderLabel)
        button = QtGui.QPushButton('Set parent folder')
        button.clicked.connect(self.browseParentFolder)
        self.expBox[-1].addRow(button)
        par = []
        for v in ['pattern','subfolders']:
            p = param[v]
            par.append(QtGui.QLineEdit(str(p)))
            self.expBox[-1].addRow(QtGui.QLabel('{0}:'.format(v)),
                                   par[-1])
        par.append(QtGui.QLineEdit(''))
        self.expBox[-1].addRow(QtGui.QLabel('Experiment Name:'),
                               par[-1])
        self.expParameterWidgets = par
        par[-1].setText(self.createExperiment(dryRun=True))
        button = QtGui.QPushButton('Create experiment')
        button.clicked.connect(self.createExperiment)
        self.expBox[-1].addRow(button)
        button = QtGui.QPushButton('Set experiment folder')
        button.clicked.connect(self.browseExperimentFolder)
        self.expBox[-1].addRow(button)
        groups[-1].setLayout(self.expBox[-1])
        # Log file
        groups.append(QtGui.QGroupBox('Experiment log'))
        self.expBox.append(QtGui.QFormLayout())
        par = []
        for v in self.config.options(logsection):
            p = self.logParameters[v]
            if p == 'user':
                p = getuser()
            par.append(QtGui.QLineEdit(p))
            self.expBox[-1].addRow(QtGui.QLabel('{0}:'.format(v)),
                                   par[-1])
        groups[-1].setLayout(self.expBox[-1])
        self.expLogWidgets = par
        expLayout = QtGui.QGridLayout(self.expTab)
        for p,g in enumerate(groups):
            expLayout.addWidget(g,0,p)
        self.expDisplay = expLayout
        self.foldername = None

    def browseParentFolder(self):
        self.parentFolder = os.path.abspath(
            str(QtGui.QFileDialog.getExistingDirectory(
                    self,"Select parent directory")))
        self.parentFolderLabel.setText(
            'Parent folder: {0}'.format(
                os.path.dirname(self.parentFolder)))

    def browseExperimentFolder(self):
        self.foldername = os.path.abspath(
            str(QtGui.QFileDialog.getExistingDirectory(
                    self,"Select experiment directory")))
        self.setExperimentFolder()

    def createExperiment(self,dryRun=False):
        global experimentFolder
        os.chdir(self.parentFolder)
        param_keys = ['pattern','subfolders','foldername']
        param = {}
        for k,p in zip(param_keys,
                       self.expParameterWidgets[:len(param_keys)]):
            param[k] = str(p.text())
        log_keys = self.config.options(logsection)
        if hasattr(self,'expLogWidgets'):
            for k,p in zip(log_keys,
                           self.expLogWidgets):
                param[k.replace(' ','_')] = str(p.text())
        # Do not create folder in the beginning
        param['subfolders'] = None
        foldername = createFoldersAndInfoFile(self.config,
                                              param,
                                              info=True,
                                              dryrun=dryRun)
        if not dryRun:
            self.foldername = os.path.abspath(foldername)
            self.setExperimentFolder()
        return foldername

    def setExperimentFolder(self):
        foldername = self.foldername
        os.chdir(self.parentFolder)
        global experimentFolder
        if not foldername is None:
            experimentFolder = os.path.abspath(foldername)
            os.chdir(experimentFolder)
            print('Setting experiment folder to: {0}'.format(experimentFolder))
            self.refreshFileModel(foldername)
        
    def outputCommander(self):
        form = QtGui.QGridLayout()
        channel = QtGui.QLineEdit(str(os.environ['AO_CHANNEL_VC']))
        form.addWidget(QtGui.QLabel('{0}:'.format('Channel')),0,0,1,1)
        form.addWidget(channel,0,1,1,1)
        form.addWidget(QtGui.QLabel('CC hold: '),1,0,1,1)
        par = QtGui.QLineEdit(str(0))
        form.addWidget(par,1,1,1,1)
        button = RunButton('lcg-output -c {0} -- {1}',
                           [channel]+[par],'CC hold',None)
        button.setText('HOLD')
        button.clicked.connect(button.runCommand)
        form.addWidget(button,1,2,1,2)
        form.addWidget(QtGui.QLabel('VC hold: '),2,0,1,1)
        par = QtGui.QLineEdit(str(-70))
        form.addWidget(par,2,1,1,1)
        button = RunButton('lcg-output -c {0} -V -- {1}',
                           [channel]+[par],'VC hold',None)
        button.setText('HOLD')
        button.clicked.connect(button.runCommand)
        form.addWidget(button,2,2,1,2)
        zeroButton = RunButton('lcg-zero',
                               [],'ZERO',None)
        zeroButton.setText('ZERO ALL CHANNELS')
        zeroButton.setStyleSheet('QPushButton {color:red; font-weight:bold}')
        zeroButton.clicked.connect(zeroButton.runCommand)
        form.addWidget(zeroButton,3,0,1,4,
                       QtCore.Qt.Alignment(QtCore.Qt.AlignTop))
        return form

    def initProtocols(self):
        prot = self.protocols
        self.protBox = []
        global externalProcessLabel
        externalProcessLabel = QtGui.QLabel('')
        protWidget = QtGui.QWidget()
        self.protTab = QtGui.QScrollArea()
        self.protTab.setWidget(protWidget)
        self.protTab.setWidgetResizable(True)
        groups = []
        self.protParameterWidgets = []
        for ii,prot in enumerate(self.protocols):
            groups.append(QtGui.QGroupBox(prot['name']))
            self.protBox.append(QtGui.QFormLayout())
            par = []
            for p,v in zip(prot['parameters'],prot['values']):
                par.append(QtGui.QLineEdit(str(v)))
                self.protBox[ii].addRow(QtGui.QLabel('{0}:'.format(p)),
                                        par[-1])
            self.protParameterWidgets.append(par)
            checkHoldTerminal = QtGui.QCheckBox('Hold term')
            checkUseLastFolder = QtGui.QCheckBox('Use last folder')
            self.protBox[ii].addRow(checkUseLastFolder)
            button = RunButton(prot['command'],par,
                               prot['name'],prot['foldername'],
                               checkHoldTerminal,checkUseLastFolder)
            button.clicked.connect(button.runCommand)
            button.setText('Run Protocol')
            self.protBox[ii].addRow(button,checkHoldTerminal)
            groups[ii].setLayout(self.protBox[ii])
        groups.append(QtGui.QGroupBox('Output Command'))
        groups[-1].setLayout(self.outputCommander())
        protLayout = QtGui.QGridLayout(protWidget)
        # Assign "optimal" number of rows and collumns
        nrows = 1
        ncol = 4
        N = len(groups)
        if N > ncol:
            if (N%ncol > 0) and (N%ncol < ncol/2):
                ncol -= 1
        else:
            ncol = N
        x = 0
        y = 0
        for p,g in enumerate(groups):
            protLayout.addWidget(g,x,y)
            y += 1
            if y == ncol:
                x +=1
                y = 0
        protLayout.addWidget(externalProcessLabel,  y + 1, 0, 1, N,
                             QtCore.Qt.Alignment(QtCore.Qt.AlignTop))
        self.protDisplay = protLayout 

    def refreshFileModel(self,rootdir = '.'):
        filters = ['*.h5']
        rootdir = os.path.abspath(rootdir)
        model = self.fileModel
        treeView = self.treeview
        model.reset()
        treeView.reset()
        model.setNameFilters(filters)
        model.setNameFilterDisables(False)
        rootIndex = model.setRootPath(rootdir)
        treeView.setModel(model)
        treeView.setRootIndex(rootIndex)
        treeView.expandAll()
        treeView.setHeaderHidden(True)
        for i in range(1,4):
            treeView.hideColumn(i)

    def initPlotting(self):
        self.plotTab = QtGui.QWidget()
        plotLayout = QtGui.QGridLayout(self.plotTab)
        # Files and browsing tree
        self.treeview = QtGui.QTreeView()
        self.fileModel = QtGui.QFileSystemModel(self.treeview)
        self.refreshFileModel()
        self.treeview.selectionModel().currentChanged.connect(
            self.handleFileSelection)
        plotLayout.addWidget(self.treeview,0,0)
        button = QtGui.QPushButton('Clear plot')
        button.clicked.connect(self.clearPlot)
        plotLayout.addWidget(button,1,0)
        # Plotting window using MatplotLib
        self.plotFigure = plt.figure()
        self.plotAxes = []
        self.plotCounter = []
        self.plotCanvas = FigureCanvas(self.plotFigure)
        self.plotToolbar = NavigationToolbar(self.plotCanvas,self)        
        plotLayout.addWidget(self.plotCanvas,0,1,2,5)
        plotLayout.addWidget(self.plotToolbar,1,1,2,5)
        self.plotDisplay = plotLayout

    def clearPlot(self):
        self.plotFigure.clf()
        self.plotAxes = []
        self.plotCounter = []
        self.plotCanvas.draw()

    @QtCore.pyqtSlot("QModelIndex")
#    @QtCore.pyqtSlot("QItemSelection, QItemSelection")
    def handleFileSelection(self, selected):
        # The plot handler.
        # This function has been canibalized so that it can work under python 2.6
        filename = os.path.abspath(str(self.fileModel.filePath(selected)))
        if (os.path.isfile(filename) and ('h5' in filename) and
            (not filename in self.plotCounter)):
            filestats = os.stat(filename)
            if filestats.st_size < 1e7:
                downsample = 1
            else:
                downsample = 2

            self.plotAxes,self.plotCounter = plotAllEntitiesFromFile(
                self.plotFigure,
                filename,
                [],
                self.plotCounter,
                self.plotAxes, downsample)
            self.plotCanvas.draw()

def main():
    mainApp = QtGui.QApplication(sys.argv)
    #['Windows', 'Motif', 'CDE', 'Plastique', 'GTK+', 'Cleanlooks']
    mainApp.setStyle(QtGui.QStyleFactory.create('Cleanlooks'))
    parser = ArgumentParser()
    parser.add_argument('-c','--config',
                        dest = 'config',
                        action = 'store',
                        default = defaultConfig)
    opts = parser.parse_args()
    if not os.path.isfile(opts.config):
        print('File {0} not found.'.format(opts.config))
        if opts.config == defaultConfig:
            print('Creating default config file.')
            createDefaultConfig(None,
                                appendDefaultParameters=True,
                                cfgfile = defaultConfig )
        else:
            sys.exit(1)
    app =  LCG_COMMANDER(cfgfile=opts.config)
    sys.exit(mainApp.exec_())

if __name__ ==  '__main__':
    main()
