#!/usr/bin/env python
import os
import sys
import numpy as np
import getopt
import lcg
from time import sleep

def usage():
    print('\nUsage: %s mode [option <value>]' % os.path.basename(sys.argv[0]))
    print('This script injects an ornstein-uhlenbeck noise process into the neuron')
    print('\n\033[94mThe global options are:\033[0m')
    print('\033[92m     -n  \033[0m  number of repetitions (default 1)')
    print('\033[92m     -i  \033[0m  interval between repetitions (default 60 s)')
    print('\033[92m     -s  \033[0m  standard deviation (default 135 pA)')
    print('\033[92m     -t  \033[0m  time constant (default 5 ms)')
    print('\033[92m     -m  \033[0m  mean (default 0 pA)')
    print('\033[92m     -d  \033[0m  duration of the noisy stimulus (default 60 s)')
    print('\033[92m     -D  \033[0m  duration of the trial (default 65 s)')
    print('\033[92m     -H  \033[0m  holding current "kernel, trial"  (default 0pA for both)')
    print('\033[92m     -I  \033[0m  input channels separated by commas (default 0)')
    print('\033[92m     -O  \033[0m  output channels separated by commas (default 0)')
    print('\033[92m     -F  \033[0m  sampling frequency (default 20000)')
    print('\033[92m     --no-kernel  \033[0m  do not compute the kernel.')
    print('\033[92m     --with-preamble  \033[0m  includes a stability preamble.')


switches = 'n:I:O:F:i:t:m:d:s:H:'
long_switches = ['no-kernel','with-preamble']

def parseArgs():
    '''
    Parses the options
    '''
    try:
        opts,args = getopt.getopt(sys.argv[1:],switches,long_switches)
    except getopt.GetoptError, err:
        print(str(err))
        usage()
        sys.exit(1)

    options = {'nreps': 1,
               'interval': 30,   # [s]
               'ai': [0],
               'ao': [0],
               'srate':20000,
               'holding':[0,0],
               'std':[135],
               'mean':[0],
               'tau':[5],
               'duration':60,
               'tail':63,
               'kernel':True,
               'preamble':False}
               
    for o,a in opts:
        if o == '-n':
            options['nreps'] = int(a)
        if o == '-m':
            options['mean'] = [np.float(i) for i in a.split(',')]
        if o == '-s':
            options['std'] = [np.float(i) for i in a.split(',')]
        if o == '-t':
            options['tau'] = [np.float(i) for i in a.split(',')]
        if o == '-H':
            options['holding'] = [np.float(i) for i in a.split(',')]
        elif o == '-i':
            options['interval'] = float(a)
        elif o == '-d':
            options['duration'] = float(a)
        elif o == '-D':
            options['tail'] = float(a)
        elif o == '-I':
            options['ai'] = [np.int(i) for i in a.split(',')]
        elif o == '-O':
            options['ao'] = [np.int(i) for i in a.split(',')]
        elif o == '-F':
            options['srate'] = float(a)
        elif o == '--no-kernel':
            options['kernel'] = False
        elif o == '--with-preamble':
            options['preamble'] = True
    options['tail'] = options['tail']-options['duration']
    if options['tail'] < 0:
        options['tail'] = 1
    return options

def create_ou_stimulus(m,s,t,d,tail,preamble):
    if not preamble:
        stimulus = [[tail,1,0,0,0,0,0,0,0,0,0,1]]
    else:
        stimulus = []
    if len(s)>1:
        stimulus.append([d,-2,m[0],s[0],t[0],0,0,0,0,2,0,1])
    else:
        stimulus.append([d,2,m[0],s[0],t[0],0,0,0,0,0,0,1])
    for mm,ss,tt in zip(m[1::],s[1::],t[1::]): 
        stimulus.append([0,-2,mm,ss,tt,0,0,0,0,2,1,1])
    stimulus.append([tail,1,0,0,0,0,0,0,0,0,0,1])
    return stimulus

def main():
    if len(sys.argv) < 2 or sys.argv[1] in ('-h','--help','help'):
        usage()
        sys.exit(0)
    
    opts = parseArgs()
    
    ncells = len(opts['ao']) 
    stim_file = []
    for ii in range(ncells):
        stim_file.append('ou_prot%d_%d.stim'%(opts['ai'][ii],opts['ao'][ii]))
        stimulus = create_ou_stimulus(opts['mean'],opts['std'],
                                      opts['tau'],opts['duration'],
                                      opts['tail'],opts['preamble'])         
        lcg.writeStimFile(stim_file[ii], stimulus, opts['preamble'])                
    for ai,ao,stim in zip(opts['ai'],opts['ao'],stim_file):
        if opts['kernel']:
            os.system('constant_output -s ' + os.environ['AO_SUBDEVICE'] +
                      ' -c ' + str(ao) + ' -v ' + str(opts['holding'][0]) + 
                      ' -f ' + os.environ['AO_CONVERSION_FACTOR'])
            if not opts['holding'][0] in [0]:
                sleep(2)
            if ncells>1:
                os.system('kernel_protocol -I ' + str(ai) + ' -O ' + str(ao) + 
                          ' -F ' + str(opts['srate']) + ' -H ' + str(opts['holding'][0]) + ' -a')
            else:
                os.system('kernel_protocol -I ' + str(ai) + ' -O ' + str(ao) + 
                          ' -F ' + str(opts['srate']) + ' -H ' + str(opts['holding'][0]))
    
        os.system('constant_output -s ' + os.environ['AO_SUBDEVICE'] +
                  ' -c ' + str(ao) + ' -v ' + str(opts['holding'][1]) + 
                  ' -f ' + os.environ['AO_CONVERSION_FACTOR'])
        if not opts['holding'][0] == opts['holding'][1]:
            sleep(2)
        os.system('cclamp -f ' + stim +  ' -i ' + str(opts['interval']) +
                  ' -F ' + str(opts['srate']) + ' -H ' + str(opts['holding'][1]))

if __name__ == '__main__':
    main()

