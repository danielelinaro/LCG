#!/usr/bin/env python

import os
import sys
import getopt
import lcg
import subprocess as sub

def usage():
    print('\nUsage: %s [option <value>]' % os.path.basename(sys.argv[0]))
    print('\nwhere options are:\n')
    print('            -a    stimulation amplitudes (default -300,50,50 pA).')
    print('            -d    stimulation duration (default 3 s).')
    print('            -I    input channel (default %s).' % os.environ['AI_CHANNEL'])
    print('            -O    output channel (default %s).' % os.environ['AO_CHANNEL'])
    print('            -F    sampling frequency (default %s Hz).' % os.environ['SAMPLING_RATE'])
    print('          --rt    use real-time system (yes or no, default %s).' % os.environ['LCG_REALTIME'])
    print(' --no-preamble    do not include stability preamble.')
    print('  --no-shuffle    do not shuffle trials.')
    print('   --no-kernel    do not compute the electrode kernel.')

def main():
    try:
        opts,args = getopt.getopt(sys.argv[1:], 'hd:a:I:O:F:', ['help','rt=','no-preamble','no-shuffle','no-kernel'])
    except getopt.GetoptError, err:
        print(str(err))
        usage()
        sys.exit(1)

    with_preamble = True
    add_amplitudes = True
    add_duration = True

    cmd = 'lcg-steps -t 0.5 -n 2 -i 0.5'
    for o,a in opts:
        if o in ['-h','--help']:
            usage()
            sys.exit(0)
        elif o == '--no-preamble':
            with_preamble = False
        else:
            if o == '-a':
                add_amplitudes = False
            elif o == '-d':
                add_duration = False
            cmd = cmd + ' ' + o + ' ' + a

    if add_amplitudes: cmd += ' -a -300,50,50'
    if add_duration: cmd += ' -d 3'
    if with_preamble: cmd += ' --with-preamble'

    sub.call(cmd, shell=True)

if __name__ == '__main__':
    main()


