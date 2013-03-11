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

__all__ = ['writeSinusoidallyModulatedOU',
           'writePulsesStimFile','writeIPlusBgGConfig', 'writeGainModulationConfig',
           'writeSinusoidsConfig', 'writeFClampConfig', 'writeSpontaneousConfig',
           'writePulsesStimFile', 'writefIStim','writeNoisyBackgroundConfig',
           'writeGStimFiles','writeStimFile',
           'substituteStrings',
           'computeRatesRatio', 'computeSynapticBackgroundCoefficients',
           'findSpikes','loadH5Trace']

import utils
from utils import writeSinusoidallyModulatedOU,writePulsesStimFile,writeIPlusBgGConfig, writeGainModulationConfig,writeSinusoidsConfig, writeFClampConfig, writeSpontaneousConfig,writePulsesStimFile, writefIStim,writeNoisyBackgroundConfig,writeGStimFiles,writeStimFile,computeRatesRatio,computeSynapticBackgroundCoefficients,findSpikes,loadH5Trace,substituteStrings

