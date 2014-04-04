#!/usr/bin/env python

import os
import sys
import getopt
from glob import glob
import numpy as np
import lcg
import aec

protocols = ['ap','vi','ramp','tau','steps','fi']

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
    print('   6) f-I curve with a PID controller')
    print('')
    print('Usage: %s [option <value>]' % os.path.basename(sys.argv[0]))
    print('')
    print('where options are:')
    print('')
    print('        -h, --help    Display this help message and exit.')
    print(' --pulse-amplitude    Amplitude of pulse for the AP protocol (in pA).')
    print('  --ramp-amplitude    Maximum injected current for the ramp protocol (in pA).')
    print('        --max-rate    Maximum firing rate (default 30 Hz).')
    print('                -F    Sampling frequency (default %s Hz).' % os.environ['SAMPLING_RATE'])
    print('                -I    Input channel (default %s).' % os.environ['AI_CHANNEL'])
    print('                -O    Output channel (default %s).' % os.environ['AO_CHANNEL'])
    print('              --rt    Use real-time system (yes or no, default %s).' % os.environ['LCG_REALTIME'])
    print(' --without <proto>    Do not run protocol <proto>. Valid protocol names are (' + ', '.join(protocols) + ').')

def parse_argv():
    try:
        opts,args = getopt.getopt(sys.argv[1:], 'hI:O:F:',
                                  ['help','pulse-amplitude=',
                                   'ramp-amplitude=','max-rate=',
                                   'rt=','without='])
    except getopt.GetoptError, err:
        print(str(err))
        usage()
        sys.exit(1)

    pulse_amplitude = None    # [pA]
    ramp_amplitude = None     # [pA]
    max_firing_rate = 30      # [Hz]

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
        elif o == '--max-rate':
            max_firing_rate = float(a)
        elif o == '--without':
            try:
                protocols.remove(a.lower())
            except:
                print('Unknown protocol: %s.' % a)
                print('Protocol must be one of (' + ', '.join(protocols) + ').')
                sys.exit(1)

    if pulse_amplitude is None:
        print('You must specify the pulse amplitude (--pulse-amplitude switch).')
        sys.exit(1)

    if ramp_amplitude is None:
        print('You must specify the ramp amplitude (--ramp-amplitude switch).')
        sys.exit(1)

    if max_firing_rate <= 0:
        print('The maximum firing rate must be positive.')
        sys.exit(1)

    return (pulse_amplitude,ramp_amplitude,max_firing_rate)

def run_command(directory, module, opts):
    command = 'lcg-foo '
    for k,v in global_opts.iteritems():
        command += '%s %s ' % (k,v)
    for k,v in opts.iteritems():
        command += '%s %s ' % (k,v)
    while command[-1] == ' ':
        command = command[:-1]
    os.chdir(directory)
    sys.argv = command.split(' ')
    module.main()
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

def create_directory(base):
    if not os.path.isdir(base):
        try:
            os.mkdir(base)
        except:
            print('Unable to create directory [%s].' % base)
            return None
    index = 1
    while True:
        subdir = '%s/%02d' % (base,index)
        if not os.path.isdir(subdir):
            try:
                os.mkdir(subdir)
            except:
                print('Unable to create directory [%s].' % subdir)
                return None
            os.chdir(subdir)
            path = os.path.abspath(os.curdir)
            break
        index += 1
    os.chdir(base_dir)
    return path

def main():
    pulse_amplitude,ramp_amplitude,max_firing_rate = parse_argv()
    for proto in protocols:
        d = create_directory(proto)
        if proto == 'ap':
            run_command(d, lcg.ap, {'-a': pulse_amplitude})
        elif proto == 'vi':
            run_command(d, lcg.vi)
        elif proto == 'ramp':
            run_command(d, lcg.ramp, {'-A': ramp_amplitude})
            rheobase = analyse_ramp(dirs['ramp'])[0]
            rheobase = 10*round(rheobase/10)
        elif proto == 'tau':
            run_command(d, lcg.tau)
        elif proto == 'steps':
            run_command(d, lcg.step, {'-n': 2, '-d': 2, '-i': 10,
                                       '-a': '%g,%g,%g' % (rheobase+50,2*rheobase,rheobase-50)})
        elif proto == 'fi':
            run_command(d, lcg.fi, {'-M': max_firing_rate, '-a': rheobase+50})

if __name__ == '__main__':
    main()
