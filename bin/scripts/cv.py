#!/usr/bin/env python

import os
import sys
import getopt
import numpy as np
import dlutils as dl

def usage():
    try:
        templateFile = os.environ['CONFIGURATIONS_PATH'] + '/spontaneous_plus_bg_G_long.xml'
    except:
        templateFile = os.environ['HOME'] + '/configurations/spontaneous_plus_bg_G_long.xml'
    print('\nUsage: %s [option <value>]' % sys.argv[0])
    print('\nwhere options are:\n')
    print('     -R    input resistance of the cell (in MOhm).')
    print('     -r    excitatory rate (default 7000 Hz).')
    print('     -V    minimum (hyperpolarized) voltage (default -50 mV).')
    print('     -v    maximum (depolarized) voltage (default -10 mV).')
    print('     -s    voltage step (default 10 mV).')
    print('     -i    interval between trials (default 20 s).')
    print('     -d    duration of each trial (default 300).')
    print('     -n    number of repetitions (default 1).')
    print('     -N    maximum number of spikes to record (default 300).')    
    print('     -f    configuration file (default [%s]).' % templateFile)
    print('')

def writeStimFile(filename, stimulus):
    with open(filename,'w') as fid:
        for row in stimulus:
            for value in row:
                fid.write(str(value)+'\t')
            fid.write('\n')

def run(Vm, Rm, rates_exc, duration=300, interval=0, configFile='cv.xml'):
    current = [[0.5,1,0,0,0,0,0,0,0,0,0,1],
               [0.01,1,-300,0,0,0,0,0,0,0,0,1],
               [0.5,1,0,0,0,0,0,0,0,0,0,1],
               [0.6,1,-100,0,0,0,0,0,0,0,0,1],
               [1,1,0,0,0,0,0,0,0,0,0,1],
               [0,1,0,0,0,0,0,0,0,0,0,1]]
    preamble = np.sum(current,0)[0]
    current[-1][0] = duration-preamble
    writeStimFile('current.stim',current)
    conductance = [[preamble,1,0,0,0,0,0,0,0,0,0,1],
                   [duration-preamble,2,0,0,0,0,0,1,0,0,0,1]]
    taus = {'exc': 5, 'inh': 10}
    for rate in rates_exc:
        for V in Vm:
            ratio = dl.computeRatesRatio(V, Rm)
            Gm_exc,Gm_inh,Gs_exc,Gs_inh = dl.computeSynapticBackgroundCoefficients(ratio, Rm, rate)
            print('Vm = %g, rates = %g Hz (exc) %g Hz (inh).' % (V, rate, rate/ratio))
            conductance[1][2] = Gm_exc
            conductance[1][3] = Gs_exc
            conductance[1][4] = taus['exc']
            writeStimFile('gexc.stim', conductance)
            conductance[1][2] = Gm_inh
            conductance[1][3] = Gs_inh
            conductance[1][4] = taus['inh']
            writeStimFile('ginh.stim', conductance)
            os.system('dclamp -V 3 -c ' + configFile)
            os.system('sleep ' + str(interval))

def main():
    try:
        opts,args = getopt.getopt(sys.argv[1:], 'hR:v:V:s:d:N:n:i:f:r:', ['help', 'output='])
    except getopt.GetoptError, err:
        print str(err)
        usage()
        sys.exit(1)

    Rm = 0               # [MOhm]
    Vmin = -50           # [mV]
    Vmax = -10           # [mV]
    Vstep = 10           # [mV]
    duration = 300       # [s]
    repetitions = 1      # [1]
    nspikes = 300        # [1]
    interval = 20        # [s]
    rate = 7000          # [Hz]
    configFile = None

    for o,a in opts:
        if o == '-h':
            usage()
            sys.exit(0)
        elif o == '-R':
            Rm = float(a)
        elif o == '-V':
            Vmin = float(a)
        elif o == '-v':
            Vmax = float(a)
        elif o == '-s':
            Vstep = float(a)
        elif o == '-d':
            duration = float(a)
        elif o == '-N':
            nspikes = int(a)
        elif o == '-n':
            repetitions = int(a)
        elif o == '-i':
            interval = float(a)
        elif o == '-f':
            configFile = a
        elif o == '-r':
            rate = float(a)

    if Rm <= 0:
        print('The input resistance must be positive.')
        usage()
        sys.exit(1)

    if Vmin >= Vmax:
        print('Vmin must be smaller than Vmax.')
        usage()
        sys.exit(1)

    if configFile == None:
        try:
            templateFile = os.environ['CONFIGURATIONS_PATH'] + '/in_vivo_like.xml'
        except:
            templateFile = os.environ['HOME'] + '/configurations/in_vivo_like.xml'
        if os.path.isfile(templateFile):
            configFile = 'cv.xml'
            dl.substituteStrings(templateFile, configFile,
                                 {'<maxCount>0</maxCount>': '<maxCount>'+str(nspikes)+'</maxCount>',
                                  '<tend>0</tend>': '<tend>'+str(duration)+'</tend>'})
        else:
            print('Default configuration file [%s] missing.' % templateFile)
            usage()
            sys.exit(2)
    
    Vm = np.arange(Vmin,Vmax+Vstep/2,Vstep)
    rates_exc = np.array([rate])
    for i in range(repetitions):
        np.random.shuffle(Vm)
        os.system('kernel_protocol')
        run(Vm, Rm, rates_exc, duration, interval, configFile)

if __name__ == '__main__':
    main()
