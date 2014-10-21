#!/usr/bin/env python
from argparse import ArgumentParser
import os 
import sys
from lcg.utils import makeIncrementingFolders
import ConfigParser
from getpass import getuser
from time import gmtime, strftime

default_file='.lcg-experiment-parameters'
infoFilename='experiment.info'
description = '''
This program generates standardized folder names. Standardization can make (semi-)automatic data analysis much simpler! Additionally it can log metadata regarding the experiment to the file {0}. Check the manual for suggestions and more information.
'''.format(infoFilename)

expsection = 'Experiment Folder Parameters'
logsection = 'Experiment Information File'
nfosection = 'Experiment Log'

defaultConfig = os.environ['HOME']+'/'+default_file
defaultPattern = 'YYYYMMDDA[001]'
defaultSubfolders = 'spontaneous,steps,tau'

append_experiment_name_and_date=True

def createDefaultParameters(cfg,only_exp=False):
    cfg.add_section(expsection)
    cfg.set(expsection,'foldername','')
    cfg.set(expsection,'pattern',defaultPattern)
    cfg.set(expsection,'subfolders',defaultSubfolders)
    if (not only_exp) and (not logsection in cfg.sections()):
        cfg.add_section(logsection)
        cfg.set(logsection,'Experimenter','user')
        cfg.set(logsection,'Project','LCG')
        cfg.set(logsection,'Cell type','pyramidal cell')
        cfg.set(logsection,'Pipette resistance','')
        cfg.set(logsection,'Bridge balance','AECoffline')
        cfg.set(logsection,'Capacitance compensation','0')

def appendGeneralArguments(parser,
                           configName = defaultConfig,
                           folderName = '', 
                           pattern=defaultPattern, 
                           subfolders = defaultSubfolders):
    parser.add_argument(
        "-c",dest='config_file',action='store',
        help='configuration file; default: {0}'.format(configName),
        default=configName)
    parser.add_argument(
        "-f",dest='foldername',action='store',
        help='foldername to start incrementing (overrides the date in the pattern); default: {0}'.format(folderName),
        default=folderName)
    parser.add_argument(
        "-p",dest='pattern',action='store',
        help='folder pattern to be followed; increment is between square brackets, default: {0}'.format(pattern),
        default=pattern)
    parser.add_argument(
        "-s",dest='subfolders',action='store',
        help='sub-directories to be created (separated by comma); default: {0}'.format(subfolders),
        default=subfolders)
    parser.add_argument(
        "--info",dest='info',action='store_true',
        help='append info file',
        default=False)
    parser.add_argument(
        "--dry-run",dest='dryrun',action='store_true',
        help='do not create folder, just show the name',
        default=False)

def createFoldersAndInfoFile(cfg, options,
                             info = False, dryrun=False):
    ''' Creates folders from a configuration file (ConfigParser object) and the options.
    The configuration file is used to get the order of the paremeters.'''
    
    if info and (logsection in cfg.sections()) and (not dryrun):
        log = {}
        for k in cfg.options(logsection):
            try:
                log[k] = options[k.replace(' ','_')]
            except:
                print('Confliting options and config file ({0}).'.format(k))
    foldername = makeIncrementingFolders(
        folderName=options['foldername'],
        folderPattern=options['pattern'],
        dryRun=dryrun)
    if not dryrun:
        if info and (logsection in cfg.sections()):
            nfofile = open('{0}/{1}'.format(os.path.abspath(foldername),
                                            infoFilename),'w')
            nfo = ConfigParser.ConfigParser()
            nfo.add_section(nfosection)
            if append_experiment_name_and_date:
                nfo.set(nfosection,'Experiment Name',foldername)
                nfo.set(nfosection,'Creation date',
                        strftime("%Y-%m-%d %H:%M:%S",
                                 gmtime()))
            for k in cfg.options(logsection):
                nfo.set(nfosection,k,log[k])
            nfo.write(nfofile)
            nfofile.close()
        for subfolder in options['subfolders'].split(','):
            os.makedirs(os.path.abspath('{0}/{1}/01'.format(foldername,
                                                            subfolder)))
    return foldername

def main():
    parser = ArgumentParser(add_help=False)
    appendGeneralArguments(parser)

    opts,unknown = parser.parse_known_args()

    cfg = ConfigParser.ConfigParser()
    if not os.path.isfile(opts.config_file) and not os.path.isfile(defaultConfig):
        cfgfile = open(defaultConfig,'w')
        createDefaultParameters(cfg, False) # Create default
        cfg.write(cfgfile)
        cfgfile.close()
        print('''Could not find config file. 
Creating a new one in {0}, please adjust the parameters.'''
              .format(defaultConfig))   
    elif not os.path.isfile(opts.config_file):
        print('Config file {0} not found.'.format(ops.config_file))
        sys.exit(1)
    cfg.read(opts.config_file)
    if not (expsection in cfg.sections()):
        cfgfile = open(opts.config_file,'w')
        createDefaultParameters(cfg, not opts.info) # Create exp section
        cfg.write(cfgfile)
        cfgfile.close()
        print('Did not find {0} section, appending to file.'.format(expsection))
    log_getopt = []
    log_opts = {}
    parser = ArgumentParser(add_help=True,description=description)
    appendGeneralArguments(parser, 
                           configName = opts.config_file,
                           folderName = opts.foldername, 
                           pattern=opts.pattern, 
                           subfolders = opts.subfolders)
    
    if opts.info and (logsection in cfg.sections()):
        for o in cfg.options(logsection):
            if cfg.get(logsection,o).lower() == 'user':
                cfg.set(logsection,o,getuser())
            parser.add_argument("--{0}".format(o.replace(' ','-')),
                                dest=o.replace(' ','_'),action='store',
                                default=cfg.get(logsection,o), 
                                help = 'default: {0}'.format(
                    cfg.get(logsection,o)))
        
    opts = parser.parse_args()
    options = vars(opts)
    foldername = createFoldersAndInfoFile(cfg, options, 
                                          info= opts.info,
                                          dryrun=opts.dryrun)
    print('{0}'.format(os.path.abspath(foldername)))
    sys.exit(0)

if __name__ == '__main__':
    main()
