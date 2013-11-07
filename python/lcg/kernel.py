
#!/usr/bin/env python

import os
import sys
import getopt
import subprocess as sub
import glob
from numpy import random as rnd
import lcg

stim_file = 'kernel.stim'

def usage():
    print('\nUsage: %s [option <value>]' % os.path.basename(sys.argv[0]))
    print('\nwhere options are:\n')
    print('     -h    display this help message and exit.')
    print('     -d    duration of the stimulation (default 10 sec).')
    print('     -s    standard deviation of the white noise (default 250 pA).')
    print('     -I    input channel (default %s).' % os.environ['AI_CHANNEL'])
    print('     -O    output channel (default %s).' % os.environ['AO_CHANNEL'])
    print('     -F    sampling frequency (default 20000 Hz).')
    print('     -H    holding current (default 0 pA).')
    print(' -a,--append append channel numbers to kernel.dat file (default no).')
    print(' --non-rt use lcg-non-rt (default no).')
    print('')

def main():
    try:
        opts,args = getopt.getopt(sys.argv[1:], 'hd:s:I:O:F:H:a', ['help','append','non-rt'])
    except getopt.GetoptError, err:
        print str(err)
        usage()
        sys.exit(1)

    ai = int(os.environ['AI_CHANNEL'])
    ao = int(os.environ['AI_CHANNEL'])
    duration = 10          # [s]
    amplitude = 250        # [pA]
    sampling_rate = 20000  # [Hz]
    holding_current = 0    # [pA]
    append = False
    nonrt = False

    for o,a in opts:
        if o in ('-h','--help'):
            usage()
            sys.exit(0)
        elif o in ('-a','--append'):
            append = True
        elif o == '-d':
            duration = float(a)
        elif o == '-s':
            amplitude = float(a)
        elif o == '-F':
            sampling_rate = float(a)
        elif o == '-I':
            ai = int(a)
        elif o == '-O':
            ao = int(a)
        elif o == '-H':
            holding_current = float(a)
        elif o == '--non-rt':
            nonrt = True

    if append:
        suffix = '-' + str(ai) + '-' + str(ao)
    else:
        suffix = ''

    stim = [[duration,11,0,amplitude,0,0,0,1,int(rnd.uniform(high=10000)),0,0,1],
            [1,1,0,0,0,0,0,0,0,0,0,1]]
    lcg.writeStimFile(stim_file,stim,True)
    if nonrt:
        fname = 'kernel.cfg'
        sub.call('lcg-rcwrite -e -i -c ' + str(ai) + ' --non-rt --file ' + fname, shell=True)
        sub.call('lcg-rcwrite -o -c ' + str(ao) + ' --non-rt --file ' + fname + ' -p ' + stim_file, shell=True)
        sub.call('lcg-non-rt -c ' + fname + ' -F ' + str(sampling_rate) + ' -H ' + str(holding_current), shell=True)
    else:
        sub.call('lcg-rcwrite -e -i -c ' + str(ai), shell=True)
        sub.call('lcg-rcwrite -o -c ' + str(ao), shell=True)
        sub.call('lcg vcclamp -f ' + stim_file + ' -F ' + str(sampling_rate) + ' -H ' + str(holding_current), shell=True)

    files = glob.glob('*.h5')
    files.sort()
    data_file = files[-1]
    lcg.computeElectrodeKernel(data_file)
    try:
        os.remove('kernel' + suffix + '.dat')
    except:
        pass
    os.symlink(data_file[:-3] + '_kernel.dat', 'kernel' + suffix + '.dat')

if __name__ == '__main__':
    main()
