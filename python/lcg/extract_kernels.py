#!/usr/bin/env python

import sys
import os
import lcg

def usage():
    print('')
    print('This script analyzes the H5 file(s) passed as argument (or all the files')
    print('in a given directory) to extract the electrode kernel required for the')
    print('Active Electrode Compensation. It assumes that the H5 files have been saved')
    print('by the lcg-kernel protocol.')
    print('')
    print('Usage:')
    print('')
    print('     %s file1 file2 ... filen' % os.path.basename(sys.argv[0]))
    print('     %s directory1 directory2 ... directoryn' % os.path.basename(sys.argv[0]))
    print('')
    print('If no argument is specified, directory is taken to be \'.\'.')
    print('')

def main():

    if len(sys.argv) == 1:
        directory = '.'
        FILES = [f for f in os.listdir('.') if f.lower().endswith('.h5')]
    elif len(sys.argv) == 2 and sys.argv[1] in ('-h','--help'):
        usage()
        sys.exit(0)
    else:
        FILES = sys.argv[1:]

    for F in FILES:
        if os.path.isdir(F):
            files = [f for f in os.listdir(F) if f.lower().endswith('.h5')]
            for f in files:
                print('Processing file [%s/%s].' % (F,f))
                try:
                    lcg.computeElectrodeKernel(F + '/' + f)
                except:
                    print('%s/%s: not a valid H5 file.' % (F,f))
        elif os.path.isfile(F):
            print('Processing file [' + F + '].')
            try:
                lcg.computeElectrodeKernel(F)
            except:
                print('%s: not a valid H5 file.' % F)
        else:
            print(F + ': no such file or directory.')
            sys.exit(1)

if __name__ == '__main__':
    main()
