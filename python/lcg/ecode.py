#!/usr/bin/env python

import os
import sys
import getopt
from glob import glob
import numpy as np
import lcg
import aec

protocols = ['ap','vi','ramp','tau','steps']

global_opts = {'-I': os.environ['AI_CHANNEL'],
               '-O': os.environ['AO_CHANNEL'],
               '-F': os.environ['SAMPLING_RATE'],
               '--rt': os.environ['LCG_REALTIME']}

base_dir = os.path.abspath(os.curdir)

def usage():
    print('This script performs a series of protocols used to characterize a')
    print('cell in terms of its electrophysiological code, or e-code.')
    print('')
    print('The applied protocols are (in this order):')
    print('   1) action potential shape')
    print('   2) V-I curve')
    print('   3) ramp to extract the rheobase')
    print('   4) time constant')
    print('   5) DC steps of current')
    print('')
    print('Usage: %s [option <value>]' % os.path.basename(sys.argv[0]))
    print('')
    print('where options are:')
    print('')
    print('        -h, --help    Display this help message and exit.')
    print(' --pulse-amplitude    Amplitude of pulse for the AP protocol (in pA).')
    print('  --ramp-amplitude    Maximum injected current for the ramp protocol (in pA).')
    print('                -F    Sampling frequency (default %s Hz).' % os.environ['SAMPLING_RATE'])
    print('                -I    Input channel (default %s).' % os.environ['AI_CHANNEL'])
    print('                -O    Output channel (default %s).' % os.environ['AO_CHANNEL'])
    print('              --rt    Use real-time system (yes or no, default %s).' % os.environ['LCG_REALTIME'])

def parse_argv():
    try:
        opts,args = getopt.getopt(sys.argv[1:], 'hI:O:F:',
                                  ['help','pulse-amplitude=','ramp-amplitude=','rt='])
    except getopt.GetoptError, err:
        print(str(err))
        usage()
        sys.exit(1)

    pulse_amplitude = None    # [pA]
    ramp_amplitude = None     # [pA]

    for o,a in opts:
        if o in ('-h','--help'):
            usage()
            sys.exit(0)
        elif o in global_opts.keys():
            global_opts[o] = a
        elif o == '--pulse-amplitude':
            pulse_amplitude = float(a)
        elif o == '--ramp-amplitude':
            ramp_amplitude = float(a)

    if pulse_amplitude is None:
        print('You must specify the pulse amplitude (--pulse-amplitude switch).')
        sys.exit(1)

    if ramp_amplitude is None:
        print('You must specify the ramp amplitude (--ramp-amplitude switch).')
        sys.exit(1)

    return (pulse_amplitude,ramp_amplitude)

def run_ap(directory, opts):
    command = 'lcg-ap '
    for k,v in global_opts.iteritems():
        command += '%s %s ' % (k,v)
    for k,v in opts.iteritems():
        command += '%s %s ' % (k,v)
    while command[-1] == ' ':
        command = command[:-1]
    os.chdir(directory)
    sys.argv = command.split(' ')
    lcg.ap.main()
    os.chdir(base_dir)

def run_vi(directory):
    command = 'lcg-vi '
    for k,v in global_opts.iteritems():
        command += '%s %s ' % (k,v)
    while command[-1] == ' ':
        command = command[:-1]
    os.chdir(directory)
    sys.argv = command.split(' ')
    lcg.vi.main()
    os.chdir(base_dir)

def run_ramp(directory, opts):
    command = 'lcg-ramp '
    for k,v in global_opts.iteritems():
        command += '%s %s ' % (k,v)
    for k,v in opts.iteritems():
        command += '%s %s ' % (k,v)
    while command[-1] == ' ':
        command = command[:-1]
    os.chdir(directory)
    sys.argv = command.split(' ')
    lcg.ramp.main()
    os.chdir(base_dir)

def run_tau(directory):
    command = 'lcg-tau '
    for k,v in global_opts.iteritems():
        command += '%s %s ' % (k,v)
    while command[-1] == ' ':
        command = command[:-1]
    os.chdir(directory)
    sys.argv = command.split(' ')
    lcg.tau.main()
    os.chdir(base_dir)

def run_steps(directory, opts):
    command = 'lcg-steps '
    for k,v in global_opts.iteritems():
        command += '%s %s ' % (k,v)
    for k,v in opts.iteritems():
        command += '%s %s ' % (k,v)
    while command[-1] == ' ':
        command = command[:-1]
    os.chdir(directory)
    sys.argv = command.split(' ')
    lcg.step.main()
    os.chdir(base_dir)

def list_h5_files(directory='.', ignore_kernels=True):
    if directory[-1] == '/':
        directory = directory[:-1]
    if not ignore_kernels:
        return glob(directory + '/*.h5')
    files = glob(directory + '/*.h5')
    h5_files = []
    for file in files:
        if not os.path.isfile(file[:-3] + '_kernel.dat'):
            h5_files.append(file)
    return h5_files

def extract_spikes(V, thresh=0.):
    return np.intersect1d(np.nonzero(V[:-1] < thresh)[0],np.nonzero(V[1:] >= thresh)[0])

def analyse_ramp(directory):
    if directory[-1] == '/':
        directory = directory[:-1]
    h5_files = list_h5_files(directory,True)
    if len(h5_files) == 0:
        print('No H5 files in %s.' % directory)
        return None
    rheobase = np.zeros(len(h5_files))
    try:
        Ke = np.loadtxt(directory + 'kernel.dat')
    except:
        Ke = None
    for i,file in enumerate(h5_files):
        entities,info = lcg.loadH5Trace(file)
        for ntt in entities:
            if ntt['name'] == 'Waveform':
                I = ntt['data']
            elif ntt['name'] == 'AnalogInput':
                if Ke is None:
                    V = ntt['data']
                else:
                    V = aec.compensate(ntt['data'],I,Ke)
        idx = extract_spikes(V)
        rheobase[i] = I[idx[0]]
    return (np.mean(rheobase),np.std(rheobase))

def create_directories():
    directories = {}
    for dir in protocols:
        if not os.path.isdir(dir):
            try:
                os.mkdir(dir)
            except:
                print('Unable to create directory [%s].' % prot)
                return None
        index = 1
        while True:
            subdir = '%s/%02d' % (dir,index)
            if not os.path.isdir(subdir):
                try:
                    os.mkdir(subdir)
                except:
                    print('Unable to create directory [%s].' % subdir)
                    return None
                os.chdir(subdir)
                directories[dir] = os.path.abspath(os.curdir)
                break
            index += 1
        os.chdir(base_dir)
    return directories

def main():
    pulse_amplitude,ramp_amplitude = parse_argv()
    dirs = create_directories()
    if dirs is None:
        sys.exit(1)
    run_ap(dirs['ap'], {'-a': pulse_amplitude})
    run_vi(dirs['vi'])
    run_ramp(dirs['ramp'], {'-A': ramp_amplitude})
    rheobase = analyse_ramp(dirs['ramp'])[0]
    rheobase = 10*round(rheobase/10)
    run_tau(dirs['tau'])
    run_steps(dirs['steps'], {'-n': 2, '-d': 2, '-i': 10,
                              '-a': '%g,%g,%g' % (rheobase+50,2*rheobase,rheobase-50)})

if __name__ == '__main__':
    main()
