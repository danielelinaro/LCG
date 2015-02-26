#!/usr/bin/env python

import os
import sys
import numpy as np
import getopt
import subprocess as sub
import lcg

def usage():
    print('This script injects a constant step of current into a cell.')
    print('')
    print('Usage: %s [option <value>]' % os.path.basename(sys.argv[0]))
    print('')
    print('where options are:')
    print('')
    print('              -a   stimulation amplitudes in the form start,[stop,step] (in pA).')
    print('              -d   stimulation duration (in seconds).')
    print('              -R   input resistance of the cell (in MOhm).')
    print('         --R-exc   firing rate of the excitatory presynaptic population (in Hz).')
    print('         --R-inh   firing rate of the inhibitory presynaptic population (in Hz).')
    print('         --V-bal   balanced voltage (in mV).')
    print('              -t   tail duration (0 pA of output after the stimulation, default 1 s)')
    print('              -n   number of repetitions of each amplitude (default 1)')
    print('              -i   interval between repetitions (default 1 s)')
    print('              -I   input channel (default %s)' % os.environ['AI_CHANNEL'])
    print('              -O   output channel (default %s)' % os.environ['AO_CHANNEL'])
    print('              -F   sampling frequency (default %s Hz)' % os.environ['SAMPLING_RATE'])
    print('              -H   holding current (default 0 pA)')
    print(' --with-preamble   include stability preamble.')
    print('    --no-shuffle   do not shuffle trials.')
    print('     --no-kernel   do not compute the electrode kernel.')
    print('')

def main():
    try:
        opts,args = getopt.getopt(sys.argv[1:],
                                  'hd:a:R:t:n:i:I:O:F:H:',
                                  ['help','with-preamble',
                                   'no-shuffle','no-kernel',
                                   'R-exc=','R-inh=','V-bal='])
    except getopt.GetoptError, err:
        print(str(err))
        usage()
        sys.exit(1)

    preamble_duration = 2.61 # [s]
    tau_exc = 5              # [ms]
    tau_inh = 10             # [ms]
    E_exc = 0                # [mV]
    E_inh = -80              # [mV]
    config_file = 'steps_with_bg.xml'
    gexc_stim_file = 'gexc.stim'
    ginh_stim_file = 'ginh.stim'
    stim_file = 'current.stim'
    ao = int(os.environ['AO_CHANNEL_CC'])
    ai = int(os.environ['AI_CHANNEL_CC'])
    sampling_rate = float(os.environ['SAMPLING_RATE'])    # [Hz]
    holding = 0    # [pA]
    with_preamble = False
    shuffle = True
    kernel = True
    nreps = 1
    duration = None    # [s]
    interval = 1       # [s]
    tail = 1           # [s]
    stim_ampl = []     # [pA]
    input_resistance = None # [MOhm]
    R_exc = None       # [Hz]
    R_inh = None       # [Hz]
    V_bal = None       # [mV]
    for o,a in opts:
        if o in ('-h','--help'):
            usage()
            sys.exit(0)
        elif o == '-d':
            duration = float(a)
        elif o == '-a':
            for amp in a.split(','):
                stim_ampl.append(float(amp))
        elif o == '--R-exc':
            R_exc = float(a)
        elif o == '--R-inh':
            R_inh = float(a)
        elif o == '--V-bal':
            V_bal = float(a)
        elif o == '-R':
            input_resistance = float(a)
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
            sampling_rate = float(a)
        elif o == '-H':
            holding = float(a)
        elif o == '--with-preamble':
            with_preamble = True
        elif o == '--no-shuffle':
            shuffle = False
        elif o == '--no-kernel':
            kernel = False

    if input_resistance is None:
        print('You must specify the input resistance of the cell (-R switch).')
        sys.exit(1)

    if R_exc is None and R_inh is None:
        print('You must specify at least one between excitatory or inhibitory presynaptic rate (--R-exc or --R-inh switches).')
        sys.exit(1)
        
    if R_inh is None and not V_bal is None:
        ratio = lcg.computeRatesRatio(Vm=V_bal, Rin=input_resistance)
        R_inh = R_exc / ratio
    elif R_exc is None and not V_bal is None:
        ratio = lcg.computeRatesRatio(Vm=V_bal, Rin=input_resistance)
        R_exc = R_inh * ratio
    elif R_exc is None or R_inh is None:
        print('You must specify the balanced voltage (--V-bal switch).')
        sys.exit(1)

    if duration is None:
        print('You must specify the duration of the stimulation (-d switch).')
        sys.exit(1)

    if len(stim_ampl) == 1:
        stim_ampl.append(stim_ampl[0])
        stim_ampl.append(1)
    elif len(stim_ampl) != 3:
        print('The amplitudes must be in the form start[,stop,step].')
        sys.exit(1)

    amplitudes = np.arange(stim_ampl[0],stim_ampl[1]+1,stim_ampl[2])
    if shuffle:
        np.random.shuffle(amplitudes)

    conductance = [[tail,1,0,0,0,0,0,0,0,0,0,1],
                   [duration,2,0,0,0,0,0,0,0,0,0,1],
                   [tail,1,0,0,0,0,0,0,0,0,0,1]]

    if with_preamble:
        stimulus = [[duration,1,holding,0,0,0,0,0,0,0,0,1],
                    [tail,1,holding,0,0,0,0,0,0,0,0,1]]
        total_dur = preamble_duration + duration + tail
        conductance[0][0] = preamble_duration
        row = 0
    else:
        stimulus = [[tail,1,holding,0,0,0,0,0,0,0,0,1],
                    [duration,1,holding,0,0,0,0,0,0,0,0,1],
                    [tail,1,holding,0,0,0,0,0,0,0,0,1]]
        total_dur = 2*tail + duration
        row = 1

    # write the configuration file
    config = lcg.XMLConfigurationFile(sampling_rate,total_dur)
    config.add_entity(lcg.entities.H5Recorder(id=0, connections=(), compress=True))
    config.add_entity(lcg.entities.RealNeuron(id=1, connections=(0), spikeThreshold=-20, V0=-65, deviceFile=os.environ['COMEDI_DEVICE'],
                                              inputSubdevice=os.environ['AI_SUBDEVICE'],
                                              outputSubdevice=os.environ['AO_SUBDEVICE'],
                                              readChannel=ai, writeChannel=ao,
                                              inputConversionFactor=os.environ['AI_CONVERSION_FACTOR_CC'],
                                              outputConversionFactor=os.environ['AO_CONVERSION_FACTOR_CC'],
                                              inputRange=os.environ['RANGE'], reference=os.environ['GROUND_REFERENCE'],
                                              kernelFile='kernel.dat'))
    config.add_entity(lcg.entities.Waveform(id=2, connections=(0,3), filename=gexc_stim_file, units='nS'))
    config.add_entity(lcg.entities.ConductanceStimulus(id=3, connections=(1), E=E_exc))
    config.add_entity(lcg.entities.Waveform(id=4, connections=(0,5), filename=ginh_stim_file, units='nS'))
    config.add_entity(lcg.entities.ConductanceStimulus(id=5, connections=(1), E=E_inh))
    config.add_entity(lcg.entities.Waveform(id=6, connections=(0,1), filename=stim_file, units='pA'))
    config.write(config_file)

    Gm_exc,Gm_inh,Gs_exc,Gs_inh = lcg.computeSynapticBackgroundCoefficients(R_exc/R_inh,R_exc,Rin=input_resistance)
    conductance[1][2] = Gm_exc
    conductance[1][3] = Gs_exc
    conductance[1][4] = tau_exc
    lcg.writeStimFile(gexc_stim_file, conductance, False)
    conductance[1][2] = Gm_inh
    conductance[1][3] = Gs_inh
    conductance[1][4] = tau_inh
    lcg.writeStimFile(ginh_stim_file, conductance, False)
    
    for _ in range(nreps):
        if kernel:
            sub.call('lcg kernel -I %d -O %d -F %g -H %g --rt yes' % (ai,ao,sampling_rate,holding), shell=True)
        for i,amp in enumerate(amplitudes):
            stimulus[row][2] = amp + holding
            lcg.writeStimFile(stim_file, stimulus, with_preamble, preamble_holding=holding)
            sub.call(lcg.common.prog_name + ' -V 4 -c ' + config_file, shell=True)
            if i < nreps*len(amplitudes)-1:
                sub.call(['sleep', str(interval)])

if __name__ == '__main__':
    main()


