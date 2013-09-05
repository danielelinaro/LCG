#!/usr/bin/env python
import os
import sys
import numpy as np
import lcg
import getopt
import subprocess as sub


purpose = '''This script allows the experimenter to deliver repetitive pulses using 
lcg-non-rt and to output also a level for synchronization with an external
recording device.'''

help = '''Usage: {} [option <value>]
Options for this script are:
    -n [2]   number of trials
    -a [2000,500]   amplitude of the pulses (pA)
    -f [1,1]    frequency of the pulses (Hz)
    -d [2,100]    duration of the pulses (ms)
    -i [0.1] interval between trials (s)
    -D [180] duration of the trial without rin probing
    -F [20000] sampling frequency(Hz)
    -I [0] input channel
    -O [0,1] output channels
    --no-rin    does not include hyperpolarizing steps
 to estimate input resistance\n
    --no-kernel    do not compute kernel before trials
    ++++++++++++++++++++++++++++++++++++++++++++++++++
    Note: With the default parameters it alternates between
 short (2.5nA, 2ms) and a long pulses (500pA, 100ms) trials.'''

switches = 'n:a:f:d:i:D:F:I:O'
long_switches = 'no-rin','no-kernel'

def parseArgs():
    '''
    Parses command line options
    '''
    try:
        opts,args = getopt.getopt(sys.argv[1:],switches,long_switches)
    except getopt.GetoptError,err:
        print(str(err))
        print(help.format(os.path.basename(sys.argv[0])))
        sys.exit(1)
    # default values
    opt = {'nreps': 2,
           'interval': 0.1,
           'dur': 180,
           'amp': [2500,500],
           'freq': [1,1],
           'pw': [2,100],
           'srate': 20000,
           'ai':[0],
           'ao':[0,1],
           'kernel': True,
           'r_in': True}
    for o,a in opts:
        if o == '-n':
            opt['nreps'] = int(a)
        elif o == '-i':
            opt['interval'] = float(a)
        elif o == '-D':
            opt['dur'] = float(a)
        elif o == '-a':
            opt['amp'] = [np.float(i) for i in a.split(',')]
        elif o == '-d':
            opt['pwd'] = [np.float(i) for i in a.split(',')]
        elif o == '-f':
            opt['freq'] = [np.float(i) for i in a.split(',')]
        elif o == '-I':
            opt['ai'] = [np.int(i) for i in a.split(',')]
        elif o == '-O':
            opt['ao'] = [np.int(i) for i in a.split(',')]
        elif o == '-F':
            opt['srate'] = float(a)
        elif o == '--no-kernel':
            opt['kernel'] = False
        elif o == '--no-rin':
            opt['r_in'] = False
    return opt

def createIntraStim(dur,amp,freq,pw,r_in):
    stim = [[1,1,0,0,0,0,0,0,0,0,0,1]]
    if r_in:
        stim.append([10,8,-150,-1,100,0,0,0,0,0,0,1])
    stim.append([1,1,0,0,0,0,0,0,0,0,0,1])
    stim.append([dur,8,amp,-freq,pw,0,0,0,0,0,0,1])
    stim.append([1,1,0,0,0,0,0,0,0,0,0,1])
    return stim

def createDigiStim(dur,r_in):
    stim = [[1,1,0,0,0,0,0,0,0,0,0,1]]
    if r_in:
        stim.append([10,1,0,0,0,0,0,0,0,0,0,1])
    stim.append([0.5,1,0,0,0,0,0,0,0,0,0,1])
    stim.append([dur+1,1,1,0,0,0,0,0,0,0,0,1])
    stim.append([0.5,1,0,0,0,0,0,0,0,0,0,1])
    return stim

def createCfg(ai,ao,fnames):
    aistr = '''[AnalogInput]
device = {}
range = [-10,+10]
subdevice = {}
channels = {}
conversionFactors = {}
reference = {}
units = mV'''
    aistr = aistr.format(os.environ['COMEDI_DEVICE'],
                         os.environ['AI_SUBDEVICE'],
                         ','.join([str(i)for i in ai]),
                         os.environ['AI_CONVERSION_FACTOR'],
                         os.environ['GROUND_REFERENCE'])
    aostr = '''[AnalogOutput]
device = {}
range = [-10,+10]
subdevice = {}
channels = {}
conversionFactors = {},5
reference = {}
units = mV
stimfiles = {}'''
    aostr = aostr.format(os.environ['COMEDI_DEVICE'],
                         os.environ['AO_SUBDEVICE'],
                         ','.join([str(i)for i in ao]),
                         os.environ['AO_CONVERSION_FACTOR'],
                         os.environ['GROUND_REFERENCE'],
                         ','.join(fnames))
    return '{}\n\n{}'.format(aistr,aostr) 


def main():
    
    if len(sys.argv) < 1:
        print(help.format(os.path.basename(sys.argv[0])))
        sys.exit(0)
    opts = parseArgs()
    if not (len(opts['amp']) == len(opts['pw']) == len(opts['freq'])):
        print('amplitude, pulsewidth and frequency must have the same number of arguments.')
        print(help.format(os.path.basename(sys.argv[0])))
        sys.exit(1)
    
    N = len(opts['amp'])
    ntrials = np.ceil(opts['nreps']/N)
    fnames = ['intra.stim','digi.stim']
    filename = 'simultaneous.cfg'
    if not len(opts['ao']) == len(fnames):
        print('Must have 2 and only 2 AO.')
        sys.exit(1)
    cfgFile = createCfg(opts['ai'],opts['ao'],fnames)
    with open(filename, "w") as fd:
        fd.write(cfgFile)
    if opts['kernel']:
        sub.call('lcg kernel -I ' + str(opts['ai'][0]) + ' -O ' + str(opts['ao'][0]) +
                 ' -F '+ str(opts['srate']) + ' -d 25 -s 150', shell=True)
    for ii in np.arange(ntrials):
        print('Trial {} of {}.'.format(int(ii),int(ntrials)))
        for nn in np.arange(N):
            lcg.writeStimFile(fnames[0],createIntraStim(opts['dur'],
                                                        opts['amp'][nn],
                                                        opts['freq'][nn],
                                                        opts['pw'][nn],
                                                        opts['r_in']),
                              addDefaultPreamble=False)
            lcg.writeStimFile(fnames[1],createDigiStim(opts['dur'],
                                                       opts['r_in']),
                              addDefaultPreamble=False)
            sub.call('lcg-non-rt -c ' + filename + ' -F '+ str(opts['srate']), shell=True)
if __name__ == '__main__':
    main()
