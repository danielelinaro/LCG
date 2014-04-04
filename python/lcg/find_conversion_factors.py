#!/usr/bin/env python

import os
import sys
from sys import stdout
import glob
import getopt
import lcg
import subprocess as sub
import numpy as np
#import matplotlib.pylab as plt
import ipdb

description='''
Finds the correct conversion factors for a particular board.
Required that the channels are properly connected and that the stimulation is ON.
Follow the instructions carefully.
The switches are:
\t --CCchannels <Input,Output> for the Current Clamp channels
\t --VCchannels <Input,Output> for the Voltage Clamp channels
\t --VCchannels <Input,Output> for the Voltage Clamp channels
\t -m <resistance> resistance of the model cell in MOhm (Default is 10MOhm)
\t Note that need to specify the channels that you will use to 
interface with the amplifier during finding the coefficients.

'''
usage='{0}\n'.format(description)

default_options={'output_filename':[],
                 'CCchannels':[0,1],
                 'VCchannels':[0,1],
                 'model_cell':10,
                 }
def read_analog_input(filename):
    ent,info = lcg.loadH5Trace(filename)
    for e in ent:
        if e['name'] in ['AnalogInput']:
            V = e['data']
    time = np.linspace(0,info['tend'],len(V))
    return V,time,info

def parse_options():
    opt = default_options.copy()
    try:
        opts,args = getopt.getopt(sys.argv[1:], 'ho:m:',['help','CCchannels=','VCchannels='])
    except:
        print(usage)
        sys.exit(1)
    for o,a in opts:
        if o in ['help','-h','--help']:
            print usage
            sys.exit(1)
        if o in ['-o']:
            opt['outout_filename'] = a
        if o in ['-m']:
            opt['model_cell'] = float(a)
        if o in ['--VCchannels']: 
            opt['VCchannels'] = [int(b) for b in a.split(',')]
        if o in ['--CCchannels']: 
            opt['CCchannels'] = [int(b) for b in a.split(',')]

    return opt
    
def main():
    opts = parse_options()
    os.chdir('/tmp')
    
    filename = 'conv_factors.stim'
    config_file = 'conv_factors.xml'
    h5_file = 'conv_factors.h5'
    expected_voltage_deflection = 10 #mV
    CCin_value = expected_voltage_deflection/opts['model_cell']
    CCout_value = 5
    VCout_value = 0.1
    duration = 0.5

    ## TODO: make that it saves always to this file always when using streams!!

    samplingRate=20000
    sub.call('lcg-zero')
    ############################# Current Clamp ##################################

    print_str = '''\nThis is a simple program to compute the conversion factors 
for your data acquisition card. This has to be done only once for each user 
(assuming you don't reset the file as when using a live cd...).

\033[32mPlease follow the instructions carefully.\033[0m
1) Prepare your amplifier to read the Current Clamp (CC) conversion factors:
\t - Connect the MODEL CELL ({0}MOhm, this is called bath mode sometimes) and respective ground.
\t\t\t You can use the -m <Resistance in MOhm> switch to specify the correct
\t\t\t resistance of your model cell. 
\t - (Zero the) offset of the amplifier.
\t - Put the amplifier in current clamp (CC) mode.
\t - Inject {1}nA with the amplifier (use the holding current).
\t - Activate the External COMMAND so that you can drive the model cell.
\t\t\t (This is not necessary in some amplifiers, like the 
\t\t\tAXON MULTICLAMP, as it is set by default).

\033[31;1m\t\t [Press ENTER when ready, this will take {2} second(s)] \033[0m
'''.format(opts['model_cell'],CCin_value,duration*2.0)
    from sys import  stdout
    stdout.write(print_str),stdout.flush()
    tmp = raw_input()

    channels = [{'type':'input',
                 'channel':opts['CCchannels'][0],
                 'factor':1.0,
                 'units':'mV'}]
    channels.append({'type':'output',
                     'channel':opts['CCchannels'][1],
                     'factor':1.0,
                     'units':'pA',
                     'stimfile':filename})
    sys.argv = ['lcg-stimgen','-o',filename,
                'dc', '-d',str(duration/2.0),'--','0',
                'dc', '-d',str(duration),'--',str(CCout_value),
                'dc', '-d',str(duration/2.0),'--','0']
    lcg.stimgen.main()
    lcg.writeIOConfigurationFile(config_file,samplingRate,
                                 duration*2.0,channels,
                                 True,recorder_filename=h5_file)

    sub.call('lcg-experiment -c {0} -V 4'.format(config_file),shell=True)

    rV,time,info = read_analog_input(h5_file)
    V_pre = rV[(time < duration/4.0)]
    V_post = rV[(time > duration) & (time<(info['tend']-duration/1.5))]
    
    CCin = 1./np.round(np.mean(V_pre)/expected_voltage_deflection,2)
    CCout = np.round((np.mean(V_post*CCin)-np.mean(V_pre*CCin))/((CCout_value*1e3)*opts['model_cell']),4)    
    ############################# Voltage Clamp ###############################
    sub.call('lcg-zero')

    print_str = '''\n
2) Prepare your amplifier for reading the Voltage Clamp conversion factors:
\t - Put the amplifier in voltage clamp (VC) mode.
\t - Inject {1}mV with the amplifier (use the holding voltage).
\t - Assure that the External COMMAND is active.

\033[31;1m\t\t [Press ENTER when ready, this will take {2} second(s)] \033[0m
'''.format(opts['model_cell'],expected_voltage_deflection ,duration*2.0)


    stdout.write(print_str);stdout.flush();tmp = raw_input()

    channels = [{'type':'input',
                 'channel':opts['VCchannels'][0],
                 'factor':1.0,
                'units':'pA'}]
    channels.append({'type':'output', 
                     'channel':opts['VCchannels'][1],
                     'factor':1.0,
                     'units':'mV',
                     'stimfile':filename})
    sys.argv = ['lcg-stimgen','-o',filename,
                'dc', '-d',str(duration/2.0),'--','0',
                'dc', '-d',str(duration),'--',str(VCout_value),
                'dc', '-d',str(duration/2.0),'--','0']
    lcg.stimgen.main()
    lcg.writeIOConfigurationFile(config_file,samplingRate,
                                 duration*2.0,channels,
                                 True,recorder_filename=h5_file)

    sub.call('lcg-experiment -c {0} -V 4'.format(config_file),shell=True)

    rI,time,info = read_analog_input(h5_file)
    I_pre = rI[(time < duration/4.0)]
    I_post = rI[(time > duration) & (time<(info['tend']-duration/1.5))]
    VCin = np.round((np.mean(I_pre)*expected_voltage_deflection)/opts['model_cell'],1)*1e3
    VCout =np.round(opts['model_cell']/(np.mean(I_post*VCin)-np.mean(I_pre*VCin)),3) 

    factors = ['AI_CONVERSION_FACTOR_CC={0}'.format(CCin),
               'AO_CONVERSION_FACTOR_CC={0}'.format(CCout),
               'AI_CONVERSION_FACTOR_VC={0}'.format(VCin),
               'AO_CONVERSION_FACTOR_VC={0}'.format(VCout)]
    print_str = '''
Make sure the following values are correct:
{0}
\n'''.format('\n'.join(factors))
    stdout.write(print_str);stdout.flush()

    stdout.write('Looking for lcg-env in .bashrc and .bash_profile\n');stdout.flush()
    lcg_env = None
    with open('{0}/.bashrc'.format(os.environ['HOME'])) as fd:
        for line in fd.readlines():
            if 'lcg-env' in line and 'source' in line:
                lcg_env = os.path.expanduser(os.path.expandvars(
                        line.split('source ')[-1].rstrip()))
    amp_name=[]
    amp_line=[]
    if not lcg_env is None:
        with open(lcg_env) as fd:
            contents = fd.readlines()
            for ii,line in enumerate(contents):
                if 'AMPLIFIER=' in line:
                    amp_name.append(line.split('AMPLIFIER="')[-1].rstrip().strip('"'))
                    amp_line.append(ii)
        stdout.write('Found lcg-env file with amplifiers: {0}\n'.format(' ,'.join(amp_name)))
        amplifier = str(raw_input('\033[36;1m Replace Amplifier or add a new one?\n\033[0m (Write amplifier name as above or write a new one.)'))
        with open(lcg_env,'r+') as fd:
            contents = fd.readlines()
            for ii,line in enumerate(contents):
                if 'AMPLIFIER=' in line:
                    if not amplifier in line.split('AMPLIFIER="')[-1].rstrip().strip('"'):
                        if not line[0] == '#':
                            contents[ii] = '#'+line
                if amplifier in amp_name:
                    if '"{0}"'.format(amplifier) in line:
                        for jj in range(4):
                            contents[ii+jj+1] = factors[jj]+'\n'
                elif amplifier.strip() not in ['']:
                    if '"HEKA-EPC10")' in line:
                        contents.insert(ii,'\t"{0}")\n'.format(amplifier))
                        for jj,a in enumerate(factors):
                            contents.insert(ii+jj+1,'\t\t'+a+'\n')
                        contents.insert(ii+jj+2,'\t\t;;\n')
                        break
                else:
                    print("Empty names not valid... Nothing was saved.")
                    break
            if not amplifier in amp_name:
                contents.insert(amp_line[0],'AMPLIFIER="{0}"\n'.format(amplifier))
            for ii,amp in enumerate(amp_name):
                if amplifier in amp:
                    contents[amp_line[ii]] = 'AMPLIFIER="{0}"\n'.format(amplifier)
            fd.seek(0)
            fd.truncate()
            [fd.write(c) for c in contents]
            stdout.flush(); stdout.write("Written to {0}.\n".format(lcg_env))
            [os.unlink(f) for f in [filename,config_file,h5_file]
if __name__ in ['__main__']:
    main()
