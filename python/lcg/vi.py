#!/usr/bin/env python

import os
import sys
import getopt
import lcg
import subprocess as sub

def usage():
    print('\nUsage: %s [option <value>]' % os.path.basename(sys.argv[0]))
    print('\nwhere options are:\n')
    print('     -a    stimulation amplitudes (-300,50,50).')
    print('     -d    stimulation duration (default 3 sec).')
    print('     -I    input channel (default 0)')
    print('     -O    output channel (default 0)')
    print('     -F    sampling frequency (default 20000)')
    print('\nAdditional options:')
    print(' --without-preamble    do not include stability preamble.')
    print(' --no-shuffle          do not shuffle trials.')
    print(' --no-kernel           do not compute the electrode kernel.')

def main():
    try:
        opts,args = getopt.getopt(sys.argv[1:], 'hd:a:I:O:F:', ['help','without-preamble','no-shuffle','no-kernel'])
    except getopt.GetoptError, err:
        print(str(err))
        usage()
        sys.exit(1)

    with_preamble = True
    add_amplitudes = True
    add_duration = True

    cmd = 'lcg-steps -t 0.1 -n 2 -i 0.5'
    for o,a in opts:
        if o in ['-h','--help']:
            usage()
            sys.exit(0)
        elif o == '--without-preamble':
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


