#!/usr/bin/env python

import sys
import os
import lcg

def parseDirectory(dir, extension = '.h5', abortIfEmpty = False):
    files = [f for f in os.listdir(dir) if f.lower().endswith(extension)]
    if len(files) == 0:
        print('No H5 files in directory [' + directory + '].')
        if abortIfEmpty:
            sys.exit(0)
    return files

def main():
    if len(sys.argv) == 1:
        directory = '.'
        files = parseDirectory(directory, abortIfEmpty=True)
    elif len(sys.argv) == 2:
        if os.path.isdir(sys.argv[1]):
            directory = sys.argv[1]
            files = parseDirectory(directory, abortIfEmpty=True)
        elif os.path.isfile(sys.argv[1]):
            files = [os.path.basename(sys.argv[1])]
            directory = os.path.dirname(sys.argv[1])
            if len(directory) == 0:
                directory = '.'
        else:
            print(sys.argv[1] + ': no such file or directory.')
            sys.exit(1)
    else:
        print('Usage: ' + sys.argv[0].split('/')[-1] + ' <file|directory>')
        sys.exit(1)

    for f in files:
        print('Processing file [' + f + '].')
        lcg.computeElectrodeKernel(directory + '/' + f)

if __name__ == '__main__':
    main()
