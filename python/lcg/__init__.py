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
           'writeSinusoidsConfig', 'writeFClampConfig', 'writeSpontaneousConfig',
           'writePulsesStimFile', 'writefIStim','writeNoisyBackgroundConfig',
           'writeGStimFiles','writeStimFile',
           'substituteStrings',
           'computeRatesRatio', 'computeSynapticBackgroundCoefficients','computeElectrodeKernel',
           'findSpikes','loadH5Trace',
           'lcg_xml']

import common
import utils
from utils import writePulsesStimFile,writeIPlusBgGConfig, writeGainModulationConfig,writeSinusoidsConfig, writeFClampConfig, writeSpontaneousConfig,writePulsesStimFile, writefIStim,writeNoisyBackgroundConfig,writeGStimFiles,writeStimFile,computeRatesRatio,computeSynapticBackgroundCoefficients,computeElectrodeKernel,findSpikes,loadH5Trace,substituteStrings
from config_writer import XMLConfigurationFile
import entities


