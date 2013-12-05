"""

=====
 lcg
=====

Collection of functions used during experiments with lcg.

Authors:
        Daniele Linaro - daniele.linaro@ua.ac.be
        Joao Couto - joao@tnb.ua.ac.be

----------------------------------------------------------

"""

__all__ = ['writePulsesStimFile','writeIPlusBgGConfig', 'writeGainModulationConfig',
           'writeSinusoidsConfig', 'writeSpontaneousConfig',
           'writePulsesStimFile', 'writefIStim','writeNoisyBackgroundConfig',
           'writeGStimFiles','writeStimFile',
           'computeRatesRatio', 'computeSynapticBackgroundCoefficients','computeElectrodeKernel',
           'findSpikes','loadH5Trace', 'lcg_xml']

import common
import utils
from utils import writePulsesStimFile, writeIPlusBgGConfig, writeGainModulationConfig, writeSinusoidsConfig, \
    writeSpontaneousConfig, writePulsesStimFile, writefIStim, writeNoisyBackgroundConfig, writeGStimFiles, \
    writeStimFile, computeRatesRatio, computeSynapticBackgroundCoefficients, computeElectrodeKernel, \
    findSpikes, loadH5Trace
from config_writer import XMLConfigurationFile, writeIOConfigurationFile
import entities
import streams

