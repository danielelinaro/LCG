#!/usr/bin/env python
import os
import sys
import numpy as np
import getopt
import lcg
from lxml import etree

def usage():
    print('\nUsage: %s mode [option <value>]' % os.path.basename(sys.argv[0]))
    print('\nThe modes are:\n')
    print('     steps             current steps are used to generate repetitive firing\n')
    print('     frequency-clamp   repetitive firing is stabilized using a PID controller\n')
    print('     noise             PRC is generated using noise injection\n')
    print('\n\nThe global options are:\n')
    print('     -n    number of repetitions (default 100)')
    print('     -i    interval between repetitions (default 1 s)\n')
    print('     -I    input channels separated by commas (default 0)')
    print('     -O    output channels separated by commas (default 0)')
    print('     -F    sampling frequency (default 20000)')
    print('\n\nThe "steps" mode options are:\n')
    print('     -a    amplitude of the perturbation (default 150 pA)')
    print('     -d    duration of the perturbation (default 1 ms)')
    print('     -w    delay of the perturbation (default 1 s)')
    print('     -A    amplitude of the step (default 100 pA)')
    print('     -D    duration of the step (default 1.5 ms)')
    print('     -t    time after and before pulse (1 s)')
    print('\nAdditional options:')
    print('     --no-kernel     does not compute the AEC kernel.')

modes = ['steps','frequency-clamp']
switches = 'n:i:I:O:F:a:A:d:D:t:w:'
long_switches = ['no-kernel','with-preamble']

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

    options = {'nreps': 1,
               'interval': 3,   # [s]
               'ai': [0],
               'ao': [0],
               'srate':20000,
               'kernel':True,
               'with_preamble':False}
    for o,a in opts:
        if o == '-n':
            options['reps'] = int(a)
        elif o == '-i':
            options['interval'] = float(a)
        elif o == '-I':
            options['ai'] = [np.int(i) for i in a.split(',')]
        elif o == '-O':
            options['ao'] = [np.int(i) for i in a.split(',')]
        elif o == '-F':
            options['F'] = float(a)
        elif o == '--no-kernel':
            options['kernel'] = False
        elif o == '--with-preamble':
            options['with_preamble'] = True
    return options

def parseStepsModeArgs():
    '''
    Parses arguments for the steps mode.
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
               'tail': 1}                  # [s]

    for o,a in opts:
        if o == '-a':
            options['pert_amplitude'] = [np.float(i) for i in a.split(',')]
        elif o == '-A':
            options['step_amplitude'] = [np.float(i) for i in a.split(',')]
        elif o == '-d':
            options['pert_width'] = [np.float(i) for i in a.split(',')]
        elif o == '-D':
            options['step_duration'] = [np.float(i) for i in a.split(',')]
        elif o == '-w':
            options['pert_delay'] = [np.float(i) for i in a.split(',')]
        elif o == '-t':
            options['tail'] = float(a)
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

def create_steps_stimulus(D,d,A,a,w,tail,with_preamble):
    if with_preamble:
        stimulus = [[D-w,1,A[0],0,0,0,0,0,0,0,0,1],
                    [d*1.e-3,1,A[0]+a,0,0,0,0,0,0,0,0,1],
                    [w-d*1.e-3,1,A[0],0,0,0,0,0,0,0,0,1],
                    [tail,1,0,0,0,0,0,0,0,0,0,1]]
    else:
        stimulus = [[tail,1,0,0,0,0,0,0,0,0,0,1],
                    [D-w,1,A,0,0,0,0,0,0,0,0,1],
                    [d*1.e-3,1,A+a,0,0,0,0,0,0,0,0,1],
                    [w-d*1.e-3,1,A,0,0,0,0,0,0,0,0,1],
                    [tail,1,0,0,0,0,0,0,0,0,0,1]]
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
        opts = dict(parseStepsModeArgs(), **opts)
        print opts['ai']
        tail = opts['tail']
        D = opts['step_duration']
        d = opts['pert_width']
        a = opts['pert_amplitude']
        A = opts['step_amplitude']
        w = opts['pert_delay']

        stim_file = list()
        for ii in range(0,len(opts['ao'])):
            print opts
            stim_file.append('prc_prot%d_%d.stim'%(opts['ai'][ii],opts['ao'][ii]))
            stimulus = create_steps_stimulus(D[ii],d[ii],A[ii],a[ii],w[ii],tail,opts['with_preamble'])         
            lcg.writeStimFile(stim_file[ii], stimulus, opts['with_preamble'])                
        if len(opts['ao']) == 1:
            if opts['kernel']:
                os.system('kernel_protocol -I ' + str(opts['ai'][0]) + ' -O ' + str(opts[['ao'][0]]) + 
                          ' -F ' + str(opts['srate']))
            os.system('cclamp -f ' + stim_file[0] +  ' -i ' + str(opts['interval']) +
                      ' -I ' + str(opts['interval']) + ' -N ' + str(opts['nreps']) + 
                      ' -F ' + str(opts['srate']))
        else:
            if opts['kernel']:
                for ai,ao in zip(opts['ai'],opts['ao']):
                    os.system('kernel_protocol -I ' + str(ai) + ' -O ' + str(ao) + 
                              ' -F ' + str(opts['srate']) + ' -a')
            # Create the configuration file.
            tmp = []
            for f in stim_file:
                tmp.append( np.sum(np.loadtxt(f),0)[0])
            opts['tend'] = max(tmp)
            root = etree.Element("dynamicclamp")
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

            for ai,ao,ff in zip(opts['ai'],opts['ao'],stim_file):

                analogInput = etree.SubElement(e_group,"entity")
                add_xml_elements(analogInput,{'name':'AnalogInput',
                                         'id':id_cnt})
                add_xml_parameters(analogInput,{'deviceFile':'dev/comedi0',
                                                'inputSubdevice':os.environ['AI_SUBDEVICE'],
                                                'inputRange': [-10,10],
                                                'readChannel': ai,
                                                'inputConversionFactor':os.environ['AI_CONVERSION_FACTOR'],
                                                'reference':os.environ['GROUND_REFERENCE']})
                add_xml_connections(analogInput,[0])
                id_cnt+=1
                analogOutput = etree.SubElement(e_group,"entity")
                add_xml_elements(analogOutput,{'name':'AnalogOutput',
                                              'id':id_cnt})
                add_xml_parameters(analogOutput,{'deviceFile':'dev/comedi0',
                                                'inputSubdevice':os.environ['AI_SUBDEVICE'],
                                                'writeChannel': ao,
                                                'outputConversionFactor':os.environ['AI_CONVERSION_FACTOR'],
                                                'reference':os.environ['GROUND_REFERENCE']})
                id_cnt+=1
                waveform = etree.SubElement(e_group,"entity")
                add_xml_elements(waveform,{'name':'Waveform',
                                              'id':id_cnt})
                add_xml_parameters(waveform,{'filename':ff,
                                             'units':'pA'})
                add_xml_connections(waveform,[0,id_cnt-1])
                id_cnt+=1

        # Write configuration file
        dclamp_tree = etree.ElementTree(root) 
        config_file = 'prc_steps.xml'
        dclamp_tree.write(config_file,pretty_print=True)
        
        os.system('dclamp -c ' + config_file +  ' -i ' + str(opts['interval']) +
                  ' -I ' + str(opts['interval']) + ' -N ' + str(opts['nreps']))
        
    elif mode in ['frequency-clamp']:

        # RealNeuron parameters
        threshold = -30
        kernel_file = 'kernel.dat'
        # Generate configuration file
        root = etree.Element("dynamicclamp")
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
                                   'deviceFile':'dev/comedi0',
                                   'inputSubdevice':os.environ['AI_SUBDEVICE'],
                                   'outputSubdevice':os.environ['AO_SUBDEVICE'],
                                   'inputRange': [-10,10],
                                   'readChannel': opts['ai'][0],
                                   'writeChannel': opts['ao'][0],
                                   'inputConversionFactor':os.environ['AI_CONVERSION_FACTOR'],
                                   'outputConversionFactor':os.environ['AO_CONVERSION_FACTOR'],
                                   'holdLastValue':True,
                                   'reference':os.environ['GROUND_REFERENCE']})
        
        # Write configuration file
        dclamp_tree = etree.ElementTree(root) 
        dclamp_tree.write('tmp.xml',pretty_print=True)
        sys.exit(1)

    else:
        print("Mode not implemented...\n")
        sys.exit(1)

    

if __name__ == '__main__':
    main()

