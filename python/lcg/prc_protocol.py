#!/usr/bin/env python

import os
import sys
import numpy as np
import getopt
import subprocess as sub
import lcg
from lxml import etree

def usage():
    print('\nUsage: %s mode [option <value>]' % os.path.basename(sys.argv[0]))
    print('\033[94m\nThe modes are:\033[0m')
    print('\033[92m    steps \033[0m            current steps are used to generate repetitive firing')
    print('     frequency-clamp   repetitive firing is stabilized using a PID controller')
    print('     noise             PRC is generated using noise injection')
    print('\n\033[94mThe global options are:\033[0m')
    print('\033[92m     -n  \033[0m  number of repetitions (default 100)')
    print('\033[92m     -i  \033[0m  interval between repetitions (default 1 s)')
    print('\033[92m     -I  \033[0m  input channels separated by commas (default 0)')
    print('\033[92m     -O  \033[0m  output channels separated by commas (default 0)')
    print('\033[92m     -F  \033[0m  sampling frequency (default %s Hz)' % os.environ['SAMPLING_RATE'])
    print('\n\033[94mThe "steps" mode options are:\033[0m')
    print('\033[92m     -a  \033[0m  amplitude of the perturbation (default 150 pA)')
    print('\033[92m     -d  \033[0m  duration of the perturbation (default 1 ms)')
    print('\033[92m     -s  \033[0m  delay of the perturbation (default 1 s)')
    print('\033[92m     -A  \033[0m  amplitude of the step (default 100 pA)')
    print('\033[92m     -D  \033[0m  duration of the step (default 1.5 ms)')
    print('\033[92m     -S  \033[0m  delay before step (does not work if preamble is used!) (1 s)')
    print('\033[92m     --extracellular  \033[0m')
    print(' \t Trigger extracellular stimulus (on the last AO)')
    print(' \t Requires an extra Analog output to be specified.\n')
    print('\033[94m\tNote:\033[0m To compute the PRC for multiple cells include' + 
          '\n\t the output channels (using the -O option) '+
          'and\n\t separate the values by commas.\n')
    print('\n\033[94mAdditional options:\033[0m')
    print('\033[92m     --no-kernel  \033[0m   does not compute the AEC kernel.')
    print('\033[92m     --with_preamble \033[0m    includes a stability preamble.')
    print('\033[94\n\nmExamples:\033[0m')
    print('Computes the PRC for 1 cell.')
    print('\033[93m\tprc_protocol steps -n 80 -I 0 -O 0 -F 30000 -i 3 -d 2 -D 1 -a 150 -A 100 -s 0.75 -S 1 --no-kernel\033[0m')
    print('Computes the PRC for 2 cells simultaneously using different parameters.')
    print('\033[93m\tprc_protocol steps -n 80 -I 0,1 -O 0,1 -F 15000 -i 5 -d 2,2 -D 1,2 -a 50 -A 100,50 -s 0.8,0.3 -S 1,1.4 --no-kernel\033[0m')
    print('Computes the PRC for 1 cell triggering an extracellular stimulus.')
    print('\033[93m\tprc_protocol steps -n 80 -I 0 -O 0,1 -F 15000 -i 3 -d 0.2 -D 1 -a 0 -A 100 -s 0.75 -S 1 --extracellular --no-kernel\033[0m')
modes = ['steps','frequency-clamp']
switches = 'n:i:I:O:F:a:A:d:D:s:S:'
long_switches = ['no-kernel','with-preamble','extracellular']

def parseGlobalArgs():
    '''
    Parses the global options
    '''
    try:
        opts,args = getopt.getopt(sys.argv[2:],switches,long_switches)
    except getopt.GetoptError, err:
        print(str(err))
        usage()
        sys.exit(1)

    options = {'nreps': 100,
               'interval': 3,   # [s]
               'ai': [0],
               'ao': [0],
               'srate':float(os.environ['SAMPLING_RATE']),
               'kernel':True,
               'with_preamble':False}
    for o,a in opts:
        if o == '-n':
            options['nreps'] = int(a)
        elif o == '-i':
            options['interval'] = float(a)
        elif o == '-I':
            options['ai'] = [np.int(i) for i in a.split(',')]
        elif o == '-O':
            options['ao'] = [np.int(i) for i in a.split(',')]
        elif o == '-F':
            options['srate'] = float(a)
        elif o == '--no-kernel':
            options['kernel'] = False
        elif o == '--with-preamble':
            options['with_preamble'] = True
    return options

def parseStepsModeArgs(n_outputs):
    '''
    Parses arguments for the steps mode.
    Requires the opts['ao'] for the number of channels
    '''
    try:
        opts,args = getopt.getopt(sys.argv[2:],switches,long_switches)
    except getopt.GetoptError, err:
        print(str(err))
        usage()
        sys.exit(1)

    options = {'pert_amplitude': [150],      # [pA]
               'pert_width': [1],            # [ms]
               'pert_delay': [1],            # [s]
               'step_duration': [1.5],       # [s]
               'step_amplitude': [100],      # [pA]
               'step_delay': [1],            # [s]
               'extracellular':False}

    for o,a in opts:
        if o == '-a':
            options['pert_amplitude'] = [np.float(i) for i in a.split(',')]
        elif o == '-A':
            options['step_amplitude'] = [np.float(i) for i in a.split(',')]
        elif o == '-d':
            options['pert_width'] = [np.float(i) for i in a.split(',')]
        elif o == '-D':
            options['step_duration'] = [np.float(i) for i in a.split(',')]
        elif o == '-s':
            options['pert_delay'] = [np.float(i) for i in a.split(',')]
        elif o == '-S':
            options['step_delay'] = [np.float(i) for i in a.split(',')]
        elif o == '--extracellular':
            options['extracellular'] = True

    if len(options['pert_amplitude']) < n_outputs:
        options['pert_amplitude'] = [options['pert_amplitude'][0]]*n_outputs 
    if len(options['step_amplitude']) < n_outputs:
        options['step_amplitude'] = [options['step_amplitude'][0]]*n_outputs 
    if len(options['pert_width']) < n_outputs:
        options['pert_width'] = [options['pert_width'][0]]*n_outputs 
    if len(options['step_duration']) < n_outputs:
        options['step_duration'] = [options['step_duration'][0]]*n_outputs 
    if len(options['pert_delay']) < n_outputs:
        options['pert_delay'] = [options['pert_delay'][0]]*n_outputs 
    if len(options['step_delay']) < n_outputs:
        options['step_delay'] = [options['step_delay'][0]]*n_outputs 
    return options

def add_xml_elements(group,parlist):
    '''
    Appends an xml Element to an etree.Element
    '''
    for a,o in parlist.iteritems():
        tmp = etree.SubElement(group, a)
        tmp.text = str(o)

def add_xml_parameters(group,parlist):
    '''
    Appends parameters to an etree.Element
    '''
    parameters = etree.SubElement(group,"parameters")
    add_xml_elements(parameters,parlist)

def add_xml_connections(entity,connections):
    connection = entity.find('connections')
    if connection is None:
        connection = etree.SubElement(entity, 'connections')
    if not connection.text is None:
        previous_conn = [str(ii) for ii in connection.text.split(',')]
        connections = np.unique(np.array([previous_conn, connections])).flatten()
    connection.text = ",".join(map(str,connections))

def create_steps_stimulus(D,d,A,a,w,s,with_preamble):
    if with_preamble:
        stimulus = [[w,1,A,0,0,0,0,0,0,0,0,1],
                    [d*1.e-3,1,A+a,0,0,0,0,0,0,0,0,1],
                    [(D-w-d*1.e-3),1,A,0,0,0,0,0,0,0,0,1],
                    [s,1,0,0,0,0,0,0,0,0,0,1]]
    else:
        stimulus = [[s,1,0,0,0,0,0,0,0,0,0,1],
                    [w,1,A,0,0,0,0,0,0,0,0,1],
                    [d*1.e-3,1,A+a,0,0,0,0,0,0,0,0,1],
                    [(D-w-d*1.e-3),1,A,0,0,0,0,0,0,0,0,1],
                    [s,1,0,0,0,0,0,0,0,0,0,1]]
        return stimulus

def main():
    mode = None
    if len(sys.argv) < 2 or sys.argv[1] in ('-h','--help','help'):
        usage()
        sys.exit(0)
    mode = sys.argv[1]
    if not mode in modes:
        print('Unknown working mode: [%s].' % sys.argv[1])
        sys.exit(1)

    opts = parseGlobalArgs()

    if mode in ['steps']:
        opts = dict(parseStepsModeArgs(len(opts['ao'])), **opts)
        D = opts['step_duration']
        d = opts['pert_width']
        a = opts['pert_amplitude']
        A = opts['step_amplitude']
        w = opts['pert_delay']        
        s = opts['step_delay']
        Ncells = len(opts['ao']) 

        if opts['extracellular']:
            opts['with_preamble'] = False
            Ncells = len(opts['ao'])-1
            if Ncells == 0:
                print('Need to specify more than one AO when using the extracellular stimulus.\n')
                sys.exit(1)
            extracellular_file = 'extracellular_prot_%d.stim'%(opts['ao'][-1])
            stimulus = create_steps_stimulus(D[1],d[1],0,7,w[1],s[1],opts['with_preamble'])
            lcg.writeStimFile(extracellular_file, stimulus,opts['with_preamble'])
            
        stim_file = list()
        for ii in range(0,Ncells):
            stim_file.append('prc_prot%d_%d.stim'%(opts['ai'][ii],opts['ao'][ii]))
            stimulus = create_steps_stimulus(D[ii],d[ii],A[ii],a[ii],w[ii],s[ii],opts['with_preamble'])         
            lcg.writeStimFile(stim_file[ii], stimulus, opts['with_preamble'])                
   
        # Use lcg; create the configuration file.
        tmp = []
        for f in stim_file:
            tmp.append( np.sum(np.loadtxt(f),0)[0])
        opts['tend'] = max(tmp)
        root = etree.Element("lcg")
        e_group = etree.SubElement(root,"entities")
        simulation_parameters = etree.SubElement(root,"simulation")
        add_xml_elements(simulation_parameters,{'rate':opts['srate'],
                                                'tend':opts['tend']})
        id_cnt = 0
        recorder = etree.SubElement(e_group,"entity")
        add_xml_elements(recorder,{'name':'H5Recorder',
                                   'id':id_cnt})
        add_xml_parameters(recorder,{'compress':True})        
        id_cnt+=1
        
        for ii in range(Ncells):#zip(opts['ai'],opts['ao'],stim_file):
                
            analogInput = etree.SubElement(e_group,"entity")
            add_xml_elements(analogInput,{'name':'AnalogInput',
                                          'id':id_cnt})
            add_xml_parameters(analogInput,{'deviceFile':'/dev/comedi0',
                                            'inputSubdevice':os.environ['AI_SUBDEVICE'],
                                            'inputRange': [-10,10],
                                            'readChannel': opts['ai'][ii],
                                            'inputConversionFactor':os.environ['AI_CONVERSION_FACTOR_CC'],
                                            'reference':os.environ['GROUND_REFERENCE']})
            add_xml_connections(analogInput,[0])
            id_cnt+=1
            analogOutput = etree.SubElement(e_group,"entity")
            add_xml_elements(analogOutput,{'name':'AnalogOutput',
                                           'id':id_cnt})
            add_xml_parameters(analogOutput,{'deviceFile':'/dev/comedi0',
                                             'outputSubdevice':os.environ['AO_SUBDEVICE'],
                                             'writeChannel': opts['ao'][ii],
                                             'outputConversionFactor':os.environ['AO_CONVERSION_FACTOR_CC'],
                                             'reference':os.environ['GROUND_REFERENCE']})
            id_cnt+=1
            waveform = etree.SubElement(e_group,"entity")
            add_xml_elements(waveform,{'name':'Waveform',
                                       'id':id_cnt})
            add_xml_parameters(waveform,{'filename':stim_file[ii],
                                         'units':'pA'})
            add_xml_connections(waveform,[0,id_cnt-1])
            id_cnt+=1

        if opts['extracellular']:
            analogOutput = etree.SubElement(e_group,"entity")
            add_xml_elements(analogOutput,{'name':'AnalogOutput',
                                           'id':id_cnt})
            add_xml_parameters(analogOutput,{'deviceFile':'/dev/comedi0',
                                             'outputSubdevice':os.environ['AO_SUBDEVICE'],
                                             'writeChannel': opts['ao'][-1],
                                             'outputConversionFactor':1,
                                             'reference':os.environ['GROUND_REFERENCE']})
            id_cnt+=1
            waveform = etree.SubElement(e_group,"entity")
            add_xml_elements(waveform,{'name':'Waveform',
                                       'id':id_cnt})
            add_xml_parameters(waveform,{'filename':extracellular_file,
                                         'units':'pA'})
            add_xml_connections(waveform,[0,id_cnt-1])
            id_cnt+=1
        # Write configuration file
        xml_tree = etree.ElementTree(root) 
        config_file = 'prc_steps.xml'
        xml_tree.write(config_file,pretty_print=True)
            # Run protocol
        if opts['kernel']:
            for ai,ao in zip(opts['ai'],opts['ao']):
                sub.call('lcg kernel ' + str(ai) + ' -O ' + str(ao) + 
                         ' -F ' + str(opts['srate']) + ' -a', shell=True)
        sub.call(lcg.common.prog_name +' -c ' + config_file +  ' -i ' + str(opts['interval']) +
                 ' -I ' + str(opts['interval']) + ' -n ' + str(opts['nreps']), shell=True)
        
    elif mode in ['frequency-clamp']:

        # RealNeuron parameters
        threshold = -30
        kernel_file = 'kernel.dat'
        # Generate configuration file
        root = etree.Element("lcg")
        e_group = etree.SubElement(root,"entities")
        simulation_parameters = etree.SubElement(root,"simulation")
        add_xml_elements(simulation_parameters,{'rate':srate,
                                                'tend':tend})
        # Insert entities
        id_cnt = 0
        recorder = etree.SubElement(e_group,"entity")
        add_xml_elements(recorder,{'name':'H5Recorder',
                                   'id':id_cnt})
        add_xml_parameters(recorder,{'compress':True})
        
        ++id_cnt
        neuron = etree.SubElement(e_group,"entity")
        add_xml_elements(neuron,{'name':'RealNeuron',
                                 'id':id_cnt})
        add_xml_parameters(neuron,{'spikeThreshold':threshold,
                                   'V0':-65,
                                   'kernelFile':kernel_file,
                                   'deviceFile':'/dev/comedi0',
                                   'inputSubdevice':os.environ['AI_SUBDEVICE'],
                                   'outputSubdevice':os.environ['AO_SUBDEVICE'],
                                   'inputRange':os.environ['RANGE'],
                                   'readChannel':opts['ai'][0],
                                   'writeChannel':opts['ao'][0],
                                   'inputConversionFactor':os.environ['AI_CONVERSION_FACTOR_CC'],
                                   'outputConversionFactor':os.environ['AO_CONVERSION_FACTOR_CC'],
                                   'holdLastValue':True,

                                   'reference':os.environ['GROUND_REFERENCE']})
        
        # Write configuration file
        xml_tree = etree.ElementTree(root) 
        xml_tree.write('tmp.xml',pretty_print=True)
        sys.exit(1)

    else:
        print("Mode not implemented...\n")
        sys.exit(1)

    

if __name__ == '__main__':
    main()

