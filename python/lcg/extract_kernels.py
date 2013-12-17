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
        files = parseDirectory(directory, abortIfEmpty=True)
    elif len(sys.argv) == 2 and sys.argv[1] in ('-h','--help'):
        usage()
        sys.exit(0)
    else:
        for arg in sys.argv[1:]:
            if os.path.isdir(arg):
                files = [f for f in os.listdir(dir) if f.lower().endswith('.h5')]
                for f in files:
                    print('Processing file [%s].' % f)
                    try:
                        lcg.computeElectrodeKernel(directory + '/' + f)
                    except:
                        print('%s: not a valid H5 file.' % (directory+'/'+f))
            elif os.path.isfile(arg):
                print('Processing file [' + arg + '].')
                try:
                    lcg.computeElectrodeKernel(arg)
                except:
                    print('%s: not a valid H5 file.' % arg)
            else:
                print(arg + ': no such file or directory.')
                sys.exit(1)

if __name__ == '__main__':
    main()
