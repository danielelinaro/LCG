#!/usr/bin/env python

import os
import sys
import getopt
import numpy as np
import lcg
import copy

waveform_codes = {'dc': 1, 'ou': 2, 'sine': 3, 'square': 4, 'saw': 5, 'chirp': 6,
                  'ramp': 7, 'poisson-reg': 8, 'poisson-exp': 9, 'poisson-bi': 10,
                  'gaussian': 11, 'alpha': 12 }

waveform_parameters = {'dc': ['amplitude'],
                       'ou': ['mean','standard deviation','time constant (ms)'],
                       'sine': ['amplitude','frequency (Hz)','phase','offset'],
                       'square': ['maximum','frequency (Hz)','duty cycle'],
                       'saw': ['maximum','frequency (Hz)','duty cycle'],
                       'chirp': ['amplitude','starting frequency (Hz)','end frequency (Hz)'],
                       'ramp': ['maximum'],
                       'poisson-reg': ['amplitude','frequency','pulse duration (ms)'],
                       'poisson-exp': ['amplitude','frequency','pulse duration (ms)'],
                       'poisson-bi': ['amplitude','frequency','pulse duration (ms)'],
                       'gaussian': ['mean','standard deviation'],
                       'alpha': ['amplitude','rise time (ms)','decay time (ms)','delay (s)', 'offset'] }

default_waveform_options = {'name': None,
                            'parameters': [None for i in range(5)],
                            'duration': None,
                            'seed': None,
                            'exponent': 1.,
                            'operator': None}

def is_positive(wavename, parameters, idx):
    for i in idx:
        if parameters[i] < 0:
            raise Exception('%s: parameter `%s\' (--p%d) must be positive.' % (wavename,waveform_parameters[wavename][i],i+1))

def square_constraints(duration, pars):
    if pars[2] > 100:
        raise Exception('square: parameter `%s\' (--p3) must be < 100.' % waveform_parameters['square'][2])
    is_positive('square', pars, (1,2))

def saw_constraints(duration, pars):
    if pars[2] > 100:
        raise Exception('saw: parameter `%s\' (--p3) must be < 100.' % waveform_parameters['saw'][2])
    is_positive('saw', pars, (1,2))

def alpha_constraints(duration, pars):
    if pars[3] > duration:
        print('alpha: parameter `%s\' must be shorter than the waveform duration.' % waveform_parameters['alpha'][3])
        return False
    return is_positive('alpha', pars, (1,2,3))

waveform_constraints = {'dc': lambda T,p: None,
                        'ou': lambda T,p: is_positive('ou', p, (1,2)),
                        'sine': lambda T,p: is_positive('sine', p, (1)),
                        'square': square_constraints,
                        'saw': saw_constraints,
                        'chirp': lambda T,p: is_positive('chirp', p, (1,2)),
                        'ramp':lambda T,p: None,
                        'poisson-reg': lambda T,p: is_positive('ou', p, (2)),
                        'poisson-exp': lambda T,p: is_positive('ou', p, (2)),
                        'poisson-bi': lambda T,p: is_positive('ou', p, (2)),
                        'gaussian': lambda T,p: is_positive('ou', p, (1)),
                        'alpha': alpha_constraints}

def usage():
    print('This script writes stimulus files that can be used by LCG to perform voltage')
    print('or current clamp experiments. Consult the STIMGEN manual to undestand which ')
    print('parameters are meaningful for each waveform.')
    print('')
    print('Usage: %s -o|--output filename [-a|--append] '  % os.path.basename(sys.argv[0]))
    print('                  <waveform_1> [option <value>] parameter_1 parameter_2 ... parameter_5')
    print('                  <waveform_2> [option <value>] parameter_1 parameter_2 ... parameter_5')
    print('                  ...')
    print('                  <waveform_N> [option <value>] parameter_1 parameter_2 ... parameter_5')
    print('')
    print('The following global options must be passed before any waveform-specific option:')
    print('')
    print('   -o, --output     output file name (if not specified, stdout is used)')
    print('   -a, --append     the waveform definition is appended to the stimulus file')
    print('')
    print('The waveform specific options are the following (note that these MUST be passed')
    print('BEFORE the positional arguments that define the parameters of the subwaveform):')
    print('')
    print('   -d, --duration   duration of the waveform, in seconds (in a composite waveform it')
    print('                    can be omitted from the second subwaveform onward)')
    print('   -s, --seed       the seed to use (default is not to fix the seed)')
    print('   -e, --exponent   exponent to which the waveform is raised (default 1)')
    print('   -p, --plus       sum this and the following waveform')
    print('   -m, --minus      subtract the followin waveformg from this one')
    print('   -M, --multiply   multiply this and the following waveform')
    print('   -D, --divide     divide this and the following waveform')
    print('   -E, --equal      compute the composite waveform: this option is required for the last waveform in a series')
    print('The parameters can also be specified with `--pi\' switches, with i ranging from 1 to 5.')
    print('')
    print('The following codes are accepted (in a case-insensitive fashion):')
    print('     dc ou sine square saw chirp ramp poisson-reg poisson-exp poisson-bi gaussian alpha')
    print('')
    print('To know which parameters are required for each waveform, type \'%s help <waveform>\'' % os.path.basename(sys.argv[0])) 
    print('')
    print('Examples:')
    print('')
    print('  Create a constant waveform of duration 1 sec and amplitude 100:')
    print('    %s -o dc.stim dc -d 1 100' % os.path.basename(sys.argv[0]))
    print('  Create a sine waveform of duration 2 sec, amplitude 50, frequency 10, phase 0 and offset 5:')
    print('    %s -o sine.stim sine --p1 50 --p2 10 --p3 0 --p4 5 -d 2' % os.path.basename(sys.argv[0]))
    print('  Append to the previous stimulus file the constant waveform of the first example:')
    print('    %s -o sine.stim -a dc -d 1 100' % os.path.basename(sys.argv[0]))
    print('  Create a composite waveform given by the multiplication of a sine with a filtered noise:')
    print('    %s -o multi.stim sine -M -d 2 1 1 0 0 ou -E 100 10 20' % os.path.basename(sys.argv[0]))
    print('')
    
def waveform_usage(waveform):
    if len(waveform_parameters[waveform]) == 1:
        print('Waveform `%s\' takes 1 parameter:' % waveform)
    else:
        print('Waveform `%s\' takes %d parameters:' % (waveform,len(waveform_parameters[waveform])))
    for i,par in enumerate(waveform_parameters[waveform]):
        print('   %d) %s (--p%d)' % (i+1,par,i+1))

def parse_global_args(argv):
    try:
        opts,args = getopt.getopt(argv, 'hao:', ['help','append','output='])
    except getopt.GetoptError, err:
        usage()
        sys.exit(1)
    options = {'append': False, 'output_filename': None}
    for o,a in opts:
        if o in ('-h','--help'):
            usage()
            sys.exit(0)
        elif o in ('-o','--output'):
            options['output_filename'] = a
        elif o in ('-a','--append'):
            options['append'] = True
    if options['append'] and options['output_filename'] is None:
        print('You cannot use -a without -o.')
        sys.exit(1)
    return options

def parse_waveform_args(argv):
    options = default_waveform_options.copy()
    options['parameters'] = copy.deepcopy(default_waveform_options['parameters'])
    options['name'] = argv[0].lower()
    try:
        opts,args = getopt.getopt(argv[1:], 'd:s:e:pmMDE', ['duration=','seed=','exponent=',
                                                           'p1=','p2=','p3=','p4=','p5=',
                                                           'plus','minus','multiply','divide','equal'])
    except getopt.GetoptError, err:
        usage()
        sys.exit(1)
    for o,a in opts:
        if o == '--p1':
            options['parameters'][0] = float(a)
        elif o == '--p2':
            options['parameters'][1] = float(a)
        elif o == '--p3':
            options['parameters'][2] = float(a)
        elif o == '--p4':
            options['parameters'][3] = float(a)
        elif o == '--p5':
            options['parameters'][4] = float(a)
        elif o in ('-d','--duration'):
            options['duration'] = np.float(a)
        elif o in ('-s','--seed'):
            options['seed'] = int(a)
        elif o in ('-e','--exponent'):
            options['exponent'] = float(a)
        elif o in ('-p','--plus'):
            if options['operator'] is None:
                options['operator'] = 'plus'
            else:
                print('You cannot specify two operators at the same time.')
                sys.exit(1)
        elif o in ('-m','--minus'):
            if options['operator'] is None:
                options['operator'] = 'minus'
            else:
                print('You cannot specify two operators at the same time.')
                sys.exit(1)
        elif o in ('-M','--multiply'):
            if options['operator'] is None:
                options['operator'] = 'multiply'
            else:
                print('You cannot specify two operators at the same time.')
                sys.exit(1)
        elif o in ('-D','--divide'):
            if options['operator'] is None:
                options['operator'] = 'divide'
            else:
                print('You cannot specify two operators at the same time.')
                sys.exit(1)
        elif o in ('-E','--equal'):
            if options['operator'] is None:
                options['operator'] = 'equal'
            else:
                print('You cannot specify two operators at the same time.')
                sys.exit(1)

    for i,p in enumerate(args):
        if not options['parameters'][i] is None:
            print('You specified twice parameter #%d, with the --p%d option (%f) and as a positional argument %f.' % 
                  (i+1,i+1,options['parameters'][i],float(p)))
            sys.exit(1)
        options['parameters'][i] = float(p)
    if options['duration'] is None and options['operator'] is None:
        print('You must specify the duration of each waveform (-d switch).')
        sys.exit(1)
    return options

def createStimulusEntry(opts):
    """
    Creates an individual entry that defines a stimulus.

    opts is a dictionary that contains the following keys:

             name - the name of the stimulus
         duration - the duration of the stimulus
             seed - the seed to use if the stimulus is stochastic (it can be None)
       parameters - a list of 5 parameters. Those that are not meaningful for the stimulus can be None
         exponent - the exponent to which the stimulus is raised
    """
    if not opts['name'] in waveform_parameters.keys():
        raise Exception('%s: unknown waveform' % opts['name'])
    if opts['duration'] is None and opts['operator'] is None:
        raise Exception('%s: missing duration.' % opts['name'])
    if opts['seed'] is None:
        fixseed = 0
        seed = 0
    else:
        fixseed = 1
        seed = opts['seed']
    stimulus = [opts['duration'],waveform_codes[opts['name']],0,0,0,0,0,fixseed,seed,0,0,opts['exponent']]
    for i in range(len(waveform_parameters[opts['name']])):
        if opts['parameters'][i] is None:
            raise Exception('%s: missing parameter `%s\' (--p%d).' % 
                            (opts['name'], waveform_parameters[opts['name']][i], i+1))
        else:
            stimulus[2+i] = opts['parameters'][i]
    # will raise an exception if something goes wrong
    waveform_constraints[opts['name']](opts['duration'],opts['parameters'])
    return stimulus

def createStimulusGroup(entries):
    operator_codes = {'plus': 1, 'minus': 3, 'multiply': 2, 'divide': 4}
    stimulus = []
    for i,entry in enumerate(entries):
        if not entry['duration'] is None and entry['duration'] != entries[0]['duration']:
            print('Ignoring duration = %g sec in entry #%d, will use %g sec from entry #1.' %
                  (entry['duration'],i,entries[0]['duration']))
        entry['duration'] = entries[0]['duration']
        stimulus.append(createStimulusEntry(entry))
    n = len(entries)
    for i in range(n):
        if i:
            stimulus[i][0] = 0
            stimulus[i][-2] = operator_codes[entries[i-1]['operator']]
        stimulus[i][-3] = stimulus[i][1]
        stimulus[i][1] = -n
    return stimulus

def writeStimFile(filename, stimulus, preamble=None, preamble_holding=0.0):
    """
    Writes a generic stimulus file.

    Parameters:
       filename - the name of the file that will be written. It can be None, in which case
                  stdout is used.
       stimulus - a matrix containing the stimulus.
       preamble - a 2 element list containing the amplitudes of a 10 ms and a 100 ms
                  long pulses to add at the beginning of the stimulation. It can be None,
                  in which case no preamble is added, or True, in which case default
                  values of -300 pA and -100 pA are used for the first and second pulse,
                  respectively.
       preamble_holding  - a number that is added to P1 of the preamble waveforms
    Returns:
       The duration of the stimulation.
           
    """
    if type(stimulus) == list and type(stimulus[0]) != list:    # a 1-dimensional list
        stimulus = [stimulus]
    if filename is None:
        fid = sys.stdout
    else:
        fid = open(filename,'w')

    preamble_dur = 0
    if preamble:
        if type(preamble) != list or len(preamble) != 2:
            preamble = [-300,-100]
        preamble = [[0.5,1,preamble_holding,0,0,0,0,0,0,0,0,1],
                    [0.01,1,preamble[0]+preamble_holding,0,0,0,0,0,0,0,0,1],
                    [0.5,1,preamble_holding,0,0,0,0,0,0,0,0,1],
                    [0.6,1,preamble[1]+preamble_holding,0,0,0,0,0,0,0,0,1],
                    [1,1,preamble_holding,0,0,0,0,0,0,0,0,1]]
        for row in preamble:
            preamble_dur = preamble_dur + row[0]
            for value in row:
                fid.write(str(value)+'\t')
            fid.write('\n')
    for row in stimulus:
        if row[0] == 0 and row[1] > 0:   # don't write lines that have zero duration
            continue
        for value in row:
            fid.write(str(value)+'\t')
        fid.write('\n')
    if fid != sys.stdout:
        fid.close()
    return preamble_dur + np.sum([row[0] for row in stimulus])
    
def main():
    if len(sys.argv) == 1:
        usage()
        sys.exit(0)

    if len(sys.argv) == 3 and sys.argv[1] in ('-h','--help','help'):
        if sys.argv[2] in waveform_parameters.keys():
            waveform_usage(sys.argv[2])
            sys.exit(0)
        else:
            print('%s: unknown waveform.' % sys.argv[2])
            sys.exit(1)

    i = 1
    while i < len(sys.argv) and not sys.argv[i].lower() in waveform_parameters.keys():
        i += 1
    global_opts = parse_global_args(sys.argv[1:i])
    if global_opts['append'] and os.path.isfile(global_opts['output_filename']):
        stimulus = np.loadtxt(global_opts['output_filename']).tolist()
        if len(stimulus) == 12 and not type(stimulus[0]) is list:
            stimulus = [stimulus]
    else:
        stimulus = []
    waveform_opts = []
    while i < len(sys.argv):
        start = i
        i += 1
        while i < len(sys.argv) and not sys.argv[i].lower() in waveform_parameters.keys():
            i += 1
        waveform_opts.append(parse_waveform_args(sys.argv[start:i]))
        if len(waveform_opts) == 1 and waveform_opts[0]['operator'] is None:
            # we just have one waveform with no operators
            stimulus.append(createStimulusEntry(waveform_opts[0]))
            waveform_opts = []
        else:
            # there are more waveforms in the list
            if waveform_opts[-1]['operator'] == 'equal':
                # everything alright: a series of operators terminated with equal
                for row in createStimulusGroup(waveform_opts):
                    stimulus.append(row)
                waveform_opts = []
            elif waveform_opts[-1]['operator'] is None or i == len(sys.argv):
                # something wrong: a series of operators followed by no operator
                # or a missing equal at the end of the series
                print('You must terminate a series of operators with the -E or --equal option.')
                sys.exit(1)

    writeStimFile(global_opts['output_filename'],stimulus,False)
                
if __name__ in ['__main__']:
    main()
