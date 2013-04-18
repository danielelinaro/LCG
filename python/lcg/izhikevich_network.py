
#!/usr/bin/env python
import os
import sys
import numpy as np
import getopt
import lcg
from lxml import etree
from numpy.random import uniform as rnd

def usage():
    print('\nUsage: %s mode [option <value>]' % os.path.basename(sys.argv[0]))
    print('Simulates a network of neurons randomly connected Izhikevich neurons using TMG synapses.')
    print('     simulation   no real neuron involved')
    print('     hybrid       interfaces with a RealNeuron (at the moment only one)')
    print('\n\033[94mThe global options are:\033[0m')
    print('\033[92m     -n  \033[0m  number of repetitions (default 100)')
    print('\033[92m     -i  \033[0m  interval between repetitions (default 1 s)')
    print('\033[92m     -F  \033[0m  sampling frequency (default 20000)')
    print('\033[92m     -N  \033[0m  number of neurons (use commas to separate between types)')
    print('\033[92m     -s  \033[0m  number of synapses (randomly picked from all neurons)')
    print('\033[92m     -w  \033[0m  synaptic weight')
    print('\033[92m     -t  \033[0m  type of synapses (0 - inhibitory,1 - excitatory)')
    print('\033[92m     -a  \033[0m  timescale of the recovery variable (pairs of values (min,max),(min,max))')
    print('\033[92m     -b  \033[0m  coupling between u and v (min max pairs)')
    print('\033[92m     -c  \033[0m  afterspike reset value of v (min max pairs)')
    print('\033[92m     -d  \033[0m  afterspike reset value of u (min max pairs)')
    print('\033[92m     --C  \033[0m  external current drive (min max pairs)')
    print('\033[92m     --record-all  \033[0m  records simulated neurons')
    print('\033[92m     --allow_autapses  \033[0m  lets the neurons connect to itself')

    print('\n\033[94mThe hybrid options are:\033[0m')
    print('\033[92m     -I  \033[0m  input channels (default 0)')
    print('\033[92m     -O  \033[0m  output channel (default 0)')

modes = ['simulation','hybrid']
switches = 'n:i:I:O:F:N:a:b:c:C:d:s:w:'
long_switches = ['no-kernel','Iext','allow_autapses','record-all']

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
               'srate':20000,
               'tend':1,
               'N':[20,5,5],
               'a':[[0.019,0.021],[0.02,0.021],[0.08,0.11]],
               'b':[[0.19,0.21],[0.25,0.26],[0.2,0.21]],
               'c':[[-65,-50],[-55,-65],[-65,-67]],
               'd':[[7,8],[2,4],[2,2.1]],
               'Vspk':[30,30,30],
               'Iext':[[1,10],[0,0],[0,1]],
               'w':[0.1,0.1,0.1],
               'synType':[1,0,0],
               'syn':[15,6,4],
               'record-all':False,
               'autapses':False,
               'kernel':False}
    for o,a in opts:
        if o == '-n':
            options['nreps'] = int(a)
        elif o == '-i':
            options['interval'] = float(a)
        elif o == '-F':
            options['srate'] = float(a)
        elif o == '-N':
            options['N'] = [int(i) for i in a.split(',')]
        elif o == '-a':
            converted_a = [np.float(i) for i in a.split(',')]
            options['a'] = [[i,j] for i,j in zip(converted_a[:-1:2],converted_a[1::2])]
        elif o == '-b':
            converted_a = [np.float(i) for i in a.split(',')]
            options['b'] = [[i,j] for i,j in zip(converted_a[:-1:2],converted_a[1::2])]
        elif o == '-c':
            converted_a = [np.float(i) for i in a.split(',')]
            options['c'] = [[i,j] for i,j in zip(converted_a[:-1:2],converted_a[1::2])]
        elif o == '-d':
            converted_a = [np.float(i) for i in a.split(',')]
            options['d'] = [[i,j] for i,j in zip(converted_a[:-1:2],converted_a[1::2])]
        elif o == '-w':
            options['w'] = [np.float(i) for i in a.split(',')]
        elif o == '-s':
            options['syn'] = [np.float(i) for i in a.split(',')]
        elif o == '-t':
            options['synType'] = [int(i) for i in a.split(',')]
        elif o == '-C':
            converted_a = [np.float(i) for i in a.split(',')]
            options['Iext'] = [[i,j] for i,j in zip(converted_a[:-1:2],converted_a[1::2])]
        elif o == '--no-kernel':
            options['kernel'] = False
        elif o == '--record-all':
            options['record-all'] = True
        elif o == '--allow-autapses':
            options['autapses'] = True
    return options

def parseHybridModeArgs():
    try:
        opts,args = getopt.getopt(sys.argv[2:],switches,long_switches)
    except getopt.GetoptError, err:
        print(str(err))
        usage()
        sys.exit(1)
    # TODO: Implement options for the RealNeuron synapses!
    options = {'ai': 0,
               'ao':0,
               'threshold':-10,
               'RN_syn_type':1,
               'RN_syn_n':20,
               'RN_syn_w':0.1}

    for o,a in opts:
        if o == '-I':
            options['ai'] = int(a)
        elif o == '-O':
            options['ao'] = int(a)

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
    if len(connections):
        connection = entity.find('connections')
        if connection is None:
            connection = etree.SubElement(entity, 'connections')
        if not connection.text is None:
            previous_conn = [str(ii) for ii in connection.text.split(',')]
            connections = np.unique(np.array([previous_conn, connections])).flatten()
        connection.text = ",".join(map(str,connections))

def add_izh_with_TMG_syn(e_group, eid, 
                         izh,
                         synCon={'delay':3e-3,'weight':1},
                         TMG={'E':0,'U':0.03,'tau1':3e-3,'tauRec':100e-3,'tauFacil':1},
                         connections=[[0],[]]):
    nrn = etree.SubElement(e_group,"entity")
    add_xml_elements(nrn,{'name':'IzhikevichNeuron',
                          'id':eid})
    add_xml_parameters(nrn,izh)        
    add_xml_connections(nrn,connections[0]) # connect to recorder if [0]
    add_xml_connections(nrn,[eid+1])

    con = etree.SubElement(e_group,"entity")
    add_xml_elements(con,{'name':'SynapticConnection',
                          'id':eid+1})
    add_xml_parameters(con,synCon)    
    add_xml_connections(con,[eid+2])
    
    syn = etree.SubElement(e_group,"entity")
    add_xml_elements(syn,{'name':'TMGSynapse',
                               'id':eid+2})
    add_xml_parameters(syn,TMG)        
    add_xml_connections(syn,connections[1])

def add_izhikevich_neuron(e_group,eid,par,connections):
    nrn = etree.SubElement(e_group,"entity")
    add_xml_elements(nrn,{'name':'IzhikevichNeuron',
                          'id':eid})
    add_xml_parameters(nrn,par)        
    add_xml_connections(nrn,connections)


def add_exponential_synapse(e_group,eid,target,delay,weight,E,tauRise,tauDecay,connections):
    
    con = etree.SubElement(e_group,"entity")
    add_xml_elements(con,{'name':'SynapticConnection',
                          'id':eid})
    add_xml_parameters(con,{'delay':delay,
                            'weight':weight})    
    add_xml_connections(con,[eid+1])
    
    syn = etree.SubElement(e_group,"entity")
    add_xml_elements(syn,{'name':'Exp2Synapse',
                               'id':eid+1})
    add_xml_parameters(syn,{'E':E,
                            'tauRise':tauRise,
                            'tauDecay':tauDecay})        
    add_xml_connections(syn,connections)
    
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
    if mode in ['hybrid']:
        opts = dict(parseHybridModeArgs(), **opts)

    recorder_id = 0
    neuron_id = []
    sfactor = 1
    offset = 1
    for i in range(len(opts['N'])):
        neuron_id.append(offset+np.arange(opts['N'][i]))
        offset = offset + opts['N'][i]
    all_neurons = [item for sublist in neuron_id for item in sublist]
    # Add RealNeuron here...
    if mode in ['hybrid']:
        real_neuron = max(all_neurons)+1
        all_neurons.append(real_neuron)
    syn = []
    for i,s in zip(range(len(neuron_id)),opts['syn']):
        idx = [np.array([all_neurons[int(rnd(max(all_neurons)))] for i in range(s)]) for n in neuron_id[i]]
        syn.append(idx)
    syn_count = max(all_neurons)+1

    syn_param = [] # [synapse_id,origin,target,weight,type]
    for nn,ss,syn_type,type_weight in zip(neuron_id,syn,opts['synType'],opts['w']):
        for n,s in zip(nn,ss):
            if not opts['autapses']:
                print n, s
                s = s[np.where(s!=n)[0]]
            weights = np.bincount(s)
            targets = np.arange(len(weights))
            targets = targets[np.nonzero(weights)]
            weights = weights[np.nonzero(weights)]
            for t,w in zip(targets,weights):
                syn_param.append([syn_count,n,t,w*type_weight,syn_type])
                syn_count += 2
    if mode in ['hybrid']:
        rn_syn_param = []
        s = np.array([all_neurons[int(rnd(max(all_neurons)))] for i in range(opts['RN_syn_n'])])
        print s
        if not opts['autapses']:
            s = s[np.where(s!=real_neuron)[0]]
        weights = np.bincount(s)
        targets = np.arange(len(weights))
        targets = targets[np.nonzero(weights)]
        weights = weights[np.nonzero(weights)]

        for t,w in zip(targets,weights):
            rn_syn_param.append([syn_count,real_neuron,t,w*opts['RN_syn_w'],opts['RN_syn_type']])
            syn_count +=2
        rn_syn_p = np.array(rn_syn_param)
    syn_p = np.array(syn_param)
    
    root = etree.Element("dynamicclamp")
    e_group = etree.SubElement(root,"entities")
    simulation_parameters = etree.SubElement(root,"simulation")
    add_xml_elements(simulation_parameters,{'rate':opts['srate'],
                                            'tend':opts['tend']})
    id_cnt = 0
    if opts['record-all'] or mode in ['hybrid']:
        recorder = etree.SubElement(e_group,"entity")
        add_xml_elements(recorder,{'name':'H5Recorder',
                                   'id':recorder_id})
        add_xml_parameters(recorder,{'compress':True})        
    
    id_cnt+=1
    # Create RealNeuron
    if mode in ['hybrid']:
        nrn = etree.SubElement(e_group,"entity")
        add_xml_elements(nrn,{'name':'RealNeuron',
                                 'id':real_neuron})
        add_xml_parameters(nrn,{'spikeThreshold':opts['threshold'],
                                   'V0':-65,
                                   'deviceFile':'/dev/comedi0',
                                   'inputSubdevice':os.environ['AI_SUBDEVICE'],
                                   'outputSubdevice':os.environ['AO_SUBDEVICE'],
                                   'inputRange': '[-10,+10]',
                                   'readChannel': opts['ai'],
                                   'writeChannel': opts['ao'],
                                   'inputConversionFactor':os.environ['AI_CONVERSION_FACTOR'],
                                   'outputConversionFactor':os.environ['AO_CONVERSION_FACTOR'],
                                   'holdLastValue':True,
                                   'reference':os.environ['GROUND_REFERENCE']})
        connections = rn_syn_p[np.where(rn_syn_p[:,1]==real_neuron)[0],0]
        connections =  np.append(connections,0)
        add_xml_connections(nrn,connections)
        for s_id,s_s,s_t,s_w,s_type in rn_syn_param:
            if s_type in [1]:
            # Excitatory
                delay = np.random.uniform(0.1e-3,10e-3)
                E = 0
                tauRise = 0.03e-3
                tauDecay = 5e-3
            else:
                # Inhibitory
                delay = np.random.uniform(0.1e-3,10e-3)
                E = -70
                tauRise = 0.1e-3
                tauDecay = 10e-3
            add_exponential_synapse(e_group,s_id,s_t,delay,s_w,E,tauRise,tauDecay,[s_t])

    # Create all Izhikevich neurons
    for ii,nn in enumerate(neuron_id):
        for n in nn:
            connections = syn_p[np.where(syn_p[:,1]==n)[0],0]
            if opts['record-all']:
               connections =  np.append(connections,0)
            izh_par = {'a': np.random.uniform(opts['a'][ii][0],opts['a'][ii][1]),
                       'b': np.random.uniform(opts['b'][ii][0],opts['b'][ii][1]),
                       'c': np.random.uniform(opts['c'][ii][0],opts['c'][ii][1]),
                       'd': np.random.uniform(opts['d'][ii][0],opts['d'][ii][1]),
                       'Vspk':opts['Vspk'][ii],
                       'Iext': np.random.uniform(opts['Iext'][ii][0],opts['Iext'][ii][1])}
            add_izhikevich_neuron(e_group,n,izh_par,connections)
    # Create all synapses
    for s_id,s_s,s_t,s_w,s_type in syn_param:
        if s_type in [1]:
            # Excitatory
            delay = np.random.uniform(0.1e-3,10e-3)
            E = 0
            tauRise = 0.03e-3
            tauDecay = 5e-3
        else:
            # Inhibitory
            delay = np.random.uniform(0.1e-3,10e-3)
            E = -70
            tauRise = 0.1e-3
            tauDecay = 10e-3
        add_exponential_synapse(e_group,s_id,s_t,delay,s_w,E,tauRise,tauDecay,[s_t])
    dclamp_tree = etree.ElementTree(root) 
    config_file = 'izhikevich_net.xml'
    dclamp_tree.write(config_file,pretty_print=True)
    # Run protocol 
    if opts['kernel']:
        os.system('kernel_protocol -I ' + str(opts['ai']) + ' -O ' + str(opts['ao']) + 
                  ' -F ' + str(opts['srate']) + ' -a')
    os.system('dclamp -c ' + config_file +  ' -i ' + str(opts['interval']) +
              ' -I ' + str(opts['interval']) + ' -n ' + str(opts['nreps']))
    


if __name__ == '__main__':
    main()

