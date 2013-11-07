#!/usr/bin/env python
import os
import sys
import getopt
import lcg
import subprocess as sub
import numpy as np
import lcg
import time

description = '''
lcg's interface to the stimgen library.
Allows quickly building single line protocols.
Check stimgen manual to undestand what parameters (P1,P2,P3)
are allowed with each waveform.
'''

stim_file = 'stimulus{0}.stim'
cfg_file = 'stimgen.cfg'

stimgen_code = {'DC': 1,
                'OU': 2,
                'sine': 3,
                'square': 4,
                'saw': 5,
                'chirp': 6,
                'ramp': 7,
                'poisson-reg': 8,
                'poisson-exp': 9,
                'poisson-bi': 10,
                'gaussian': 11,
                'alpha': 12 }

stimgen_param = {'DC': ['Amplitude'],
                'OU': ['Mean','SD','Decay'],
                'sine': ['Amplitude','Frequency','Phase'],
                'square': ['Maximum','Frequency','Duty-cycle'],
                'saw': ['Maximum','Frequency','Duty-cycle'],
                'chirp': ['Amplitude','Start Freq','End Freq'],
                'ramp': ['Maximum'],
                'poisson-reg': ['Amplitude','Frequency','Pulse-width'],
                'poisson-exp': ['Amplitude','Frequency','Pulse-width'],
                'poisson-bi': ['Amplitude','Frequency','Pulse-width'],
                'gaussian': ['Mean','SD'],
                'alpha': ['Amplitude','Rise time','Decay time'] }

opt_description = {'--P1': 'Parameter 1 (supports iteration [start,stop,step])',
                   '--P2': 'Parameter 2 (supports iteration [start,stop,step])',
                   '--P3': 'Parameter 3 (supports iteration [start,stop,step])',
                   '-o': 'offset between stimulus (if present)',
                   '-t': 'Time before stimulus',
                   '-I': 'Input channel(s)',
                   '-O': 'Output channel(s)',
                   '-F': 'Sampling frequency',
                   '--If': 'Input conversion factor(s)',
                   '--Of': 'Output conversion factor(s)',
                   '--Iu': 'Output unit(s)',
                   '--Ou': 'Output unit(s)',
                   '--seed': 'Fixes the seed to the specified value',
                   '--no-kernel': 'do not compute the kernel.',
                   '--Ks': 'std of the kernel.'}

# order of the parameters is random... is there a way to fix it??
usage = '''
Usage: {0} <protocol> [--option <value>]
{1}
Protocols:
\t{2}
Options:
\t{3}
Examples:
None at the moment...

'''.format(os.path.basename(sys.argv[0]),
           description,'\n\t'.join(stimgen_code.keys()),
           '\n\t'.join('%s \t\t %s'% t for t in opt_description.iteritems()))

opt_defaults = {'P1':None,
                'P2':None,
                'P3':None,
                'pre':1,
                'srate':20000,
                'dur':1,
                'nreps':1,
                'interval':1,
                'input_channels':[int(os.environ['AI_CHANNEL'])],
                'output_channels':[int(os.environ['AO_CHANNEL'])],
                'input_factors': None,
                'output_factors': None,
                'input_units': None,
                'output_units': None,
                'offset':None,
                'seed':None,
                'kernel': True,
                'kernel_s':200 }

def parse_options():
    options = opt_defaults.copy()
    try:
        opts,args = getopt.getopt(sys.argv[2:], 'h:o:t:n:d:i:I:O:F:', 
                                  ['help','If=','Of=','Iu=','Ou=',
                                   'seed=','no-kernel','P1=','P2=','P3=','Ks='])
    except getopt.GetoptError, err:
        print usage
        print(str(err))
        sys.exit(1)
    for o,a in opts:
        if o == '-n':
            options['nreps'] = int(a)
        if o == '-o':
            options['offset'] = np.float(a)
        if o == '--P1':
            options['P1'] = [np.float(i) for i in a.split(',')]
        if o == '--P2':
            options['P2'] = [np.float(i) for i in a.split(',')]
        if o == '--P3':
            options['P3'] = [np.float(i) for i in a.split(',')]
        if o == '-t':
            options['pre'] = np.float(a)
        if o == '-d':
            options['dur'] = np.float(a)
        if o == '-i':
            options['interval'] = np.float(a)
        if o == '--seed':
            options['seed'] = int(a)
        if o == '-I':
            options['input_channels'] = [int(i) for i in a.split(',')]
        if o == '-O':
            options['output_channels'] = [int(i) for i in a.split(',')]
        if o == '-F':
            options['srate'] = np.float(a)
        if o == '--If':
            options['input_factors'] = [np.float(i) for i in a.split(',')]
        if o == '--Of':
            options['output_factors'] = [np.float(i) for i in a.split(',')]
        if o == '--Iu':
            options['input_units'] = [i for i in a.split(',')]
        if o == '--Ou':
            options['output_units'] = [i for i in a.split(',')]
        if o == '--no-kernel':
            options['kernel'] = False
        if o == '--Ks':
            options['kernel_s'] = np.float(a)
    return options

def print_code_help(code):
    print('Code {0}({1}) takes {2} parameters:'.format(code,
                                                       stimgen_code[code],
                                                       len(stimgen_param[code])))         
    for ii in range(len(stimgen_param[code])):
        print('\t --P{0}: {1}'.format(ii+1,stimgen_param[code][ii]))
    sys.exit(1)

def check_options(opts,code):
    ''' Check that all parameters have been defined for each code. '''
    nparam = len(stimgen_param[code])
    parameters = [opts[p] for p in ['P1','P2','P3']]
    if not len(np.where(parameters)[0]) == nparam:
        print_code_help(code)
        
    if None in [opts['output_channels'],opts['input_channels']]:
        print('You need to specify at least one input and one output channel.\n')
        sys.exit(1)
    # Get default parameters.
    if opts['offset'] is None:
        opts['offset'] = 0.0
    if opts['input_factors'] is None:
        opts['input_factors'] = [os.environ['AI_CONVERSION_FACTOR']]
    if opts['output_factors'] is None:
        opts['output_factors'] = [os.environ['AO_CONVERSION_FACTOR']]
    if opts['input_units'] is None:
        opts['input_units'] = [os.environ['INPUT_UNITS']]
    if opts['output_units'] is None:
        opts['output_units'] = [os.environ['OUTPUT_UNITS']]
    # Duplicate if not sufficient
    N = len(opts['input_channels'])
    if not len(opts['input_factors']) == N:
        opts['input_factors'] = opts['input_factors'][0]*N
    if not len(opts['input_units']) == N:
        opts['input_units'] = opts['input_units'][0]*N
    N = len(opts['output_channels'])
    if not len(opts['output_factors']) == N:
        opts['output_factors'] = opts['output_factors'][0]*N
    if not len(opts['output_units']) == N:
        opts['output_units'] = opts['output_units'][0]*N
    opts['code'] = code
    
def create_stimulus_matrix(opts,p1=0,p2=0,p3=0,seed=26021985):
    if p1 is None:
        p1 = 0
    if p2 is None:
        p2 = 0
    if p3 is None:
        p3 = 0
    basestim = [0,1,0,0,0,0,0,0,seed,0,0,1]
    stim = []
    dur = []  
    for ii,ch in enumerate(opts['output_channels']):
#        print("Creating stimulus {0} for channel {1}".format(ii,ch))
        tmp = []
        tmp.append(list(basestim))
        tmp.append(list(basestim))
        tmp[0][0] = opts['pre'] + ii*opts['offset']
        tmp[1][0] = opts['dur']
        tmp[1][1] = stimgen_code[opts['code']]
        tmp[1][2] = p1
        tmp[1][3] = p2
        tmp[1][4] = p3
        stim.append(tmp)
        dur.append(tmp[0][0]+tmp[0][1])
    for d,s in zip(dur,stim):
        s.append(list(basestim))
        s[-1][0] = max(dur) - d + opts['pre']
    return stim,max(dur)+ opts['pre']

def main():
    if len(sys.argv)<2:
        print usage
        sys.exit(1)
    code = sys.argv[1]
    if not code in stimgen_code:
        print usage
        print 'Invalid stimgen code.'
        sys.exit(1)
    opts = parse_options()
    check_options(opts,code)
    # If have more than one channel, create non-rt configuration file.
    comma = lambda y:",".join([str(i) for i in y])
    run = lambda p:sub.call(p,shell=True)
    #run = lambda p:sys.stdout.write(str(p)+'\n')
    stimnames = ",".join([stim_file.format(i) for i in range(len(opts['output_channels']))])

    if len(opts['input_channels']) + len(opts['output_channels']) > 2:
        print("Using lcg-non-rt.\n")
        # write configuration file
        run('lcg-rcwrite -e -i -c {0} -u {1} -f {2} --file {3} --non-rt'.format(comma(opts['input_channels']),
                                                                       comma(opts['input_units']),
                                                                       comma(opts['input_factors']),
                                                                       cfg_file))
        run('lcg-rcwrite -o -c {0} -u {1} -f {2} -p {3} --file {4} --non-rt'.format(comma(opts['output_channels']),
                                                                           comma(opts['output_units']),
                                                                           comma(opts['output_factors']),
                                                                           stimnames,
                                                                           cfg_file))
        # compute kernels
        if opts['kernel']:
            if len(opts['output_channels']) == len(opts['input_channels']):
                for i,o in enumerate(opts['output_channels']):
                    run('lcg-kernel -s {0} -I {1} -O {2} --append --non-rt -F {3}'.format(opts['kernel_s'],
                                                                                          opts['input_channels'][i],
                                                                                          o,
                                                                                          opts['srate']))
        cmd = 'lcg-non-rt -c {0} -F {1} -n {2} -i {3}'.format(cfg_file,
                                                       opts['srate'],
                                                       opts['nreps'],
                                                       opts['interval'])
    else:
        if opts['kernel']:
            run('lcg-kernel -s {0} -I {1} -O {2} -F {3}'.format(opts['kernel_s'],
                                                                         opts['input_channels'][0],
                                                                         opts['output_channels'][0],
                                                                         opts['srate']))
        cmd = 'lcg-vcclamp -c {0} -F {1}'.format(stimnames,opts['srate'])
        print("Using lcg-vcclamp.\n")

    # Iterate through parameters 
    P1 = []
    P2 = []
    P3 = []
    if not opts['P1'] is None:
        if len(opts['P1']) < 2:
            P1 = [opts['P1'][0]]
        else:
            P1 = np.arange(opts['P1'][0],opts['P1'][1],opts['P1'][2])
    if not opts['P2'] is None:
        if len(opts['P2']) < 2:
            P2 = [opts['P2'][0]]
        else:
          P2 = np.arange(opts['P2'][0],opts['P2'][1],opts['P2'][2])
    if not opts['P3'] is None:
        if len(opts['P3']) < 2:
            P3 = [opts['P3'][0]]
        else:
            P3 = np.arange(opts['P3'][0],opts['P3'][1],opts['P3'][2])
    if not len(P1):P1=[0]
    if not len(P2):P2=[0]
    if not len(P3):P3=[0]
    for p1 in P1:
        for p2 in P2:
            for p3 in P3:
                stim,dur = create_stimulus_matrix(opts,p1,p2,p3)
                for i,s in enumerate(stim):
                        lcg.writeStimFile(stim_file.format(i),s)
                timeStart = time.time()
                run(cmd)
                if not (p1 == P1[-1] and p2 == P2[-1] and p3 == P3[-1]):
                    timeToWait = (timeStart + dur + opts['interval']) - time.time()
                    print('Waiting: {0}'.format(timeToWait))
                    if timeToWait > 0:
                        time.sleep(timeToWait)
                
if __name__ in ['__main__']:
    main()
