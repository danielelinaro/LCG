#!/usr/bin/env python

import os
import sys
import numpy as np
import getopt
import subprocess as sub
import lcg

stimuli_directory = 'stimuli'

def usage():
    print('This script can be used to generate voltage or current steps.')
    print('')
    print('Usage: %s [option <value>]' % os.path.basename(sys.argv[0]))
    print('')
    print('where options are:')
    print('')
    print('              -a   stimulation amplitudes in the form start,[stop,step]')
    print('              -d   stimulation duration (in seconds)')
    print('              -t   tail duration (duration of the output after the stimulation, ' + \
          'default 1 s in current clamp, 0.1 s in voltage clamp)')
    print('              -n   number of repetitions of each amplitude (default 1)')
    print('              -i   interval between repetitions (default 1 s)')
    print('              -I   input channel (default %s in current clamp, %s in voltage clamp)' % \
          (os.environ['AI_CHANNEL_CC'],os.environ['AI_CHANNEL_VC']))
    print('              -O   output channel (default %s in current clamp, %s in voltage clamp)' % \
          (os.environ['AO_CHANNEL_CC'],os.environ['AO_CHANNEL_VC']))
    print('              -F   sampling frequency (default %s Hz)' % os.environ['SAMPLING_RATE'])
    print('              -H   holding current or voltage (default 0 pA in current clamp or -70 mV in voltage clamp)')
    print('            --rt   use real-time system (yes or no, default %s)' % os.environ['LCG_REALTIME'])
    print('    --input-gain   input conversion factor (default %s for current clamp, %s for voltage clamp)' \
              % (os.environ['AI_CONVERSION_FACTOR_CC'],os.environ['AI_CONVERSION_FACTOR_VC']))
    print('   --output-gain   output conversion factor (default %s for current clamp, %s for voltage clamp).' \
              % (os.environ['AO_CONVERSION_FACTOR_CC'],os.environ['AO_CONVERSION_FACTOR_VC']))
    print('   --input-units   input units (default %s for current clamp, %s for voltage clamp)' \
              % (os.environ['AI_UNITS_CC'],os.environ['AI_UNITS_VC']))
    print('  --output-units   output units (default %s for current clamp, %s for voltage clamp)' \
              % (os.environ['AO_UNITS_CC'],os.environ['AO_UNITS_VC']))
    print('        --vclamp   record the cell in voltage clamp mode')
    print(' --with-preamble   include stability preamble')
    print('    --no-shuffle   do not shuffle trials (default yes in voltage clamp mode)')
    print('     --no-kernel   do not compute the electrode kernel (default yes in voltage clamp mode)')
    print('         --model   use a leaky integrate-and-fire model')
    print('')

def main():
    try:
        opts,args = getopt.getopt(sys.argv[1:],
                                  'hd:a:t:n:i:I:O:F:H:',
                                  ['help','with-preamble','vclamp',
                                   'no-shuffle','no-kernel','model','rt=',
                                   'input-gain=','output-gain=','input-units=','output-units='])
    except getopt.GetoptError, err:
        print(str(err))
        usage()
        sys.exit(1)

    suffix = 'CC'
    ao = None
    ai = None
    samplf = float(os.environ['SAMPLING_RATE'])    # [Hz]
    holding = None
    with_preamble = None
    shuffle = None
    kernel = None
    nreps = 1
    duration = None    # [s]
    interval = 1       # [s]
    tail = None        # [s]
    stim_ampl = []     # [pA]
    realtime = os.environ['LCG_REALTIME']
    model = ''
    input_gain = None
    output_gain = None
    input_units = None
    output_units = None

    for o,a in opts:
        if o in ['-h','--help']:
            usage()
            sys.exit(0)
        elif o == '-d':
            duration = float(a)
        elif o == '-a':
            for amp in a.split(','):
                stim_ampl.append(float(amp))
        elif o == '-t':
            tail = float(a)
        elif o == '-n':
            nreps = int(a)
        elif o == '-i':
            interval = float(a)
        elif o == '-I':
            ai = int(a)
        elif o == '-O':
            ao = int(a)
        elif o == '-F':
            samplf = float(a)
        elif o == '-H':
            holding = float(a)
        elif o == '--vclamp':
            suffix = 'VC'
        elif o == '--with-preamble':
            with_preamble = True
        elif o == '--no-shuffle':
            shuffle = False
        elif o == '--no-kernel':
            kernel = False
        elif o == '--model':
            model = '--model'
            realtime = 'yes'
            kernel = False
        elif o == '--rt':
            realtime = a
        elif o == '--input-gain':
            input_gain = float(a)
        elif o == '--output-gain':
            output_gain = float(a)
        elif o == '--input-units':
            input_units = a
        elif o == '--output-units':
            output_units = a

    # default values
    if suffix == 'CC':
        if holding is None:
            holding = 0
        if with_preamble is None:
            with_preamble = False
        if shuffle is None:
            shuffle = True
        if kernel is None:
            kernel = True
        if tail is None:
            tail = 1
        if input_gain is None:
            input_gain = float(os.environ['AI_CONVERSION_FACTOR_CC'])
        if output_gain is None:
            output_gain = float(os.environ['AO_CONVERSION_FACTOR_CC'])
        if input_units is None:
            input_units = 'mV'
        if output_units is None:
            output_units = 'pA'
    elif suffix == 'VC':
        if holding is None:
            try:
                holding = float(os.environ['VC_HOLDING_POTENTIAL'])
            except:
                holding = -70
        if with_preamble is None:
            with_preamble = False
        if shuffle is None:
            shuffle = False
        if kernel is None:
            kernel = False
        if tail is None:
            tail = 0.1
        if input_gain is None:
            input_gain = float(os.environ['AI_CONVERSION_FACTOR_VC'])
        if output_gain is None:
            output_gain = float(os.environ['AO_CONVERSION_FACTOR_VC'])
        if input_units is None:
            input_units = 'pA'
        if output_units is None:
            output_units = 'mV'
        
    if ai is None:
        ai = int(os.environ['AI_CHANNEL_{0}'.format(suffix)])
    if ao is None:
        ao = int(os.environ['AO_CHANNEL_{0}'.format(suffix)])
        
    if duration is None:
        print('You must specify the duration of the stimulation (-d switch)')
        sys.exit(1)

    if len(stim_ampl) == 1:
        stim_ampl.append(stim_ampl[0])
        stim_ampl.append(1)
    elif len(stim_ampl) != 3:
        print('The amplitudes must be in the form start[,stop,step].')
        sys.exit(1)

    amplitudes = np.arange(stim_ampl[0],stim_ampl[1]+stim_ampl[2]/2,stim_ampl[2])
    if shuffle:
        np.random.shuffle(amplitudes)

    try:
        os.mkdir(stimuli_directory,0755)
    except:
        ok = raw_input('The directory ['+stimuli_directory+'] already exists: shall I continue (deleting its content)? [y/N] ')
        if ok != 'y':
            sys.exit(0)
        for f in os.listdir(stimuli_directory):
            try:
                if os.path.isfile(os.path.join(stimuli_directory,f)):
                    os.unlink(os.path.join(stimuli_directory,f))
            except e:
                print e
                sys.exit(1)

    if with_preamble:
        if suffix == 'CC':
            stimulus = [[duration,1,holding,0,0,0,0,0,0,0,0,1],
                        [tail,1,holding,0,0,0,0,0,0,0,0,1]]
            row = 0
        else:
            stimulus = [[0.02,1,holding,0,0,0,0,0,0,0,0,1],
                        [duration,1,holding-10,0,0,0,0,0,0,0,0,1],
                        [duration,1,holding,0,0,0,0,0,0,0,0,1],
                        [tail,1,holding,0,0,0,0,0,0,0,0,1]]
            row = 2
            with_preamble = False
    else:
        stimulus = [[tail,1,holding,0,0,0,0,0,0,0,0,1],
                    [duration,1,holding,0,0,0,0,0,0,0,0,1],
                    [tail,1,holding,0,0,0,0,0,0,0,0,1]]
        row = 1

    for i,amp in enumerate(amplitudes):
        if suffix == 'CC':
            stimulus[row][2] = amp + holding
        else:
            stimulus[row][2] = amp
        lcg.writeStimFile('%s/step_%02d.stim' % (stimuli_directory,i+1), stimulus, 
                          with_preamble, preamble_holding=holding)

    if kernel:
        sub.call('lcg kernel -I %d -O %d -F %g -H %g --rt %s' % (ai,ao,samplf,holding,realtime), shell=True)

    command = 'lcg stimulus -d %s -i %g -I %d -g %f -u %s -O %d -G %f -U %s -n %d -F %g --rt %s %s' % \
              (stimuli_directory,interval,ai,input_gain,input_units,ao,output_gain,output_units,nreps,samplf,realtime,model)
    if suffix == 'VC':
        command += ' --voltage-clamp'

    sub.call(command, shell=True)

if __name__ == '__main__':
    main()


