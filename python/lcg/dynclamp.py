#!/usr/bin/env python

import lcg
import os
import sys
import time
import numpy as np
import subprocess as sub
import getopt

stim_file = 'current.stim'
xml_file = 'dynclamp.xml'
progname = os.path.basename(sys.argv[0])

PSUM2_TO_SCM2 = 1e-4

def write_xml_file(duration,srate,area,gbar_Na=30,gbar_K=50,model_cell=False):
    config = lcg.XMLConfigurationFile(srate,duration)
    config.add_entity(lcg.entities.H5Recorder(id=0, connections=(), compress=True))
    if model_cell:
        # don't need a kernel file when using a model cell
        config.add_entity(lcg.entities.RealNeuron(id=1, connections=(0), spikeThreshold=-40, V0=-70,
                                                  deviceFile=os.environ['COMEDI_DEVICE'],
                                                  inputSubdevice=os.environ['AI_SUBDEVICE'],
                                                  outputSubdevice=os.environ['AO_SUBDEVICE'],
                                                  readChannel=os.environ['AI_CHANNEL_CC'],
                                                  writeChannel=os.environ['AO_CHANNEL_CC'],
                                                  inputConversionFactor=os.environ['AI_CONVERSION_FACTOR_CC'],
                                                  outputConversionFactor=os.environ['AO_CONVERSION_FACTOR_CC'],
                                                  inputRange=os.environ['RANGE'],
                                                  reference=os.environ['GROUND_REFERENCE']))
    else:
        config.add_entity(lcg.entities.RealNeuron(id=1, connections=(0), spikeThreshold=-40, V0=-70,
                                                  deviceFile=os.environ['COMEDI_DEVICE'],
                                                  inputSubdevice=os.environ['AI_SUBDEVICE'],
                                                  outputSubdevice=os.environ['AO_SUBDEVICE'],
                                                  readChannel=os.environ['AI_CHANNEL_CC'],
                                                  writeChannel=os.environ['AO_CHANNEL_CC'],
                                                  inputConversionFactor=os.environ['AI_CONVERSION_FACTOR_CC'],
                                                  outputConversionFactor=os.environ['AO_CONVERSION_FACTOR_CC'],
                                                  inputRange=os.environ['RANGE'],
                                                  reference=os.environ['GROUND_REFERENCE'],
                                                  kernelFile='kernel.dat'))
    config.add_entity(lcg.entities.Waveform(id=2, connections=(0,1), filename=stim_file, units='pA'))
    config.add_entity(lcg.entities.HH2Sodium(id=3, connections=(0,1), area=area, gbar=gbar_Na*PSUM2_TO_SCM2,
                                             E=60., vtraub=-53., temperature=33))
    if model_cell:
        config.add_entity(lcg.entities.HH2Potassium(id=4, connections=(0,1), area=area, gbar=gbar_K*PSUM2_TO_SCM2,
                                                    E=-90., vtraub=-53., temperature=33))
    config.write(xml_file)

def write_stim_file(amp,dur=1,before=1,after=1):
    stimulus = [[before,1,0,0,0,0,0,0,0,0,0,1],
                [dur,1,amp,0,0,0,0,0,0,0,0,1],
                [after,1,0,0,0,0,0,0,0,0,0,1]]
    lcg.writeStimFile(stim_file, stimulus)

def usage():
    print('\nThis script injects pulses of current on top of a simulated sodium current.')
    print('It can optionally be used with a model cell, in which case also a potassium')
    print('current is injected.')
    print('')
    print('Usage: %s [option <value>] start[,stop,step]' % progname)
    print('')
    print('where start is the initial (or only) value of current (in pA) to be injected,')
    print('and the optional stop and step parameters indicate the final value of current')
    print('and the step. If the values of current to be injected are negative, use a')
    print('double hyphen (--) to separate optional arguments and current amplitudes (see')
    print('below for an example).')
    print('')
    print('Additional options are:')
    print('')
    print('    -h,--help    Display this help message and exit.')
    print('    -A,--area    The area of the neuron\'s membrane (in um2).')
    print('    --gbar-na    Maximal conductance of the sodium current (default 30 pS/um2).')
    print('     --gbar-k    Maximal conductance of the potassium current (default 50 pS/um2,')
    print('                 used only if the --model-cell option is present).')
    print('           -d    Duration of the stimulation (default 0.5 sec).')
    print('           -b    Time before the application of the stimulation (default 0.25 sec).')
    print('           -a    Time after the application of the stimulation (default 0.25 sec).')
    print(' --model-cell    Indicates that the experiment will be performed with a model')
    print('                 cell. An additional potassium current will be simulated and the')
    print('                 area option is not required.')
    print('    --shuffle    Indicates that current amplitudes should be shuffled.')
    print('')
    print('Examples:')
    print('')
    print('1) inject a 0.5 s-long pulse of amplitude 100 pA in a real cell of area')
    print('   30000 um2:')
    print('')
    print('      %s -A 30000 100' % progname)
    print('')
    print('2) inject 1 s-long pulses of amplitude -100, -50 and 0 pA in a real cell')
    print('   of area 20000 um2:')
    print('')
    print('      %s -A 20000 -d 1 -- -100,0,50' % progname)
    print('')
    print('3) change the default value of the sodium conductance in a real cell of')
    print('   area 10000 um2:')
    print('')
    print('      %s -A 10000 --gbar-na 100 200' % progname)
    print('')
    print('4) inject a 2 s-long pulse of amplitude 300 pA in a model cell with 1 s')
    print('   before and after the stimulation:')
    print('')
    print('      %s --model-cell -d 2 -b 1 -a 1 300' % progname)
    print('')

def main():
    try:
        opts,args = getopt.getopt(sys.argv[1:], 'hb:a:d:A:', ['help','model-cell','shuffle',
                                                              'area=','gbar-na=','gbar-k='])
    except getopt.GetoptError, err:
        print('%s: %s. Type \'%s -h\' for help.' % (progname,str(err),progname))
        sys.exit(1)

    area = None         # [um2]
    before = 0.25       # [s]
    after = 0.25        # [s]
    dur = 0.5           # [s]
    gbar_Na = 30.       # [pS/um2]
    gbar_K = 50.        # [pS/um2]
    srate = 15000.      # [Hz]
    model_cell = False
    shuffle = False

    for o,a in opts:
        if o in ('-h','--help'):
            usage()
            sys.exit(0)
        if o in ('-A','--area'):
            area = float(a)
        elif o == '--gbar-na':
            gbar_Na = float(a)
        elif o == '--gbar-k':
            gbar_K = float(a)
        elif o == '-b':
            before = float(a)
        elif o == '-a':
            after = float(a)
        elif o == '-d':
            dur = float(a)
        elif o == '--model-cell':
            model_cell = True
        elif o == '--shuffle':
            shuffle = True
            
    if not model_cell:
        if area is None:
            print('You must specify the area of the neuron\'s membrane (-A option).')
            sys.exit(1)
        elif area <= 0:
            print('The area of the neuron\'s membrane must be positive.')
            sys.exit(1)
    elif area is None:
        area = 30000.
        
    if gbar_Na < 0:
        print('The maximal sodium conductance must be non-negative (--gbar-na option).')
        sys.exit(0)

    if model_cell and gbar_K < 0:
        print('The maximal potassium conductance must be non-negative (--gbar-k option).')
        sys.exit(0)

    for val,lbl,opt in zip([before,after,dur],['before','after','of'],['-b','-a','-d']):
        if val < 0:
            print('Time %s stimulation must be greater than zero (%s option).' % (lbl,opt))

    if len(args) != 1:
        print('You must specify the amplitude(s) of the injected current as start[,stop,step].')
        sys.exit(1)

    try:
        amplitudes = map(float, args[0].split(','))
        if len(amplitudes) == 3:
            amplitudes = np.arange(amplitudes[0],amplitudes[1]+amplitudes[2]/2,amplitudes[2])
            if shuffle:
                np.random.shuffle(amplitudes)
        elif len(amplitudes) != 1:
            print('You must specify the amplitude(s) of the injected current as start[,stop,step].')
            sys.exit(1)
    except:
        print('You must specify the amplitude(s) of the injected current as start[,stop,step].')
        sys.exit(1)

    if not model_cell:
        sub.call('lcg-kernel')
    write_xml_file(duration=dur+before+after,srate=srate,area=area,gbar_Na=gbar_Na,gbar_K=gbar_K,model_cell=model_cell)
    for amp in amplitudes:
        write_stim_file(amp, dur, before, after)
        sub.call('lcg-experiment -c ' + xml_file, shell=True)
        if not model_cell and amp != amplitudes[-1]:
            time.sleep(dur*10)

if __name__ == '__main__':
    main()
