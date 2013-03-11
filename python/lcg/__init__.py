"""

lcg
======

Python tools for use with lcg.

For a list of available names, try dir(lcg).

---------------------

$Id: __init__.py,v 0.1 2013/01/16 15:43:47 joao$

"""

__all__ = ['writeSinusoidallyModulatedOU',
           'writePulsesStimFile','writeIPlusBgGConfig', 'writeGainModulationConfig',
           'writeSinusoidsConfig', 'writeFClampConfig', 'writeSpontaneousConfig',
           'writePulsesStimFile', 'writefIStim','writeNoisyBackgroundConfig',
           'writeGStimFiles','writeStimFile',
           'substituteStrings','findConfigurationFile','makeOutputFilename',
           'computeRatesRatio', 'computeSynapticBackgroundCoefficients','computeElectrodeKernel',
           'findSpikes',
           'loadH5Trace','loadH5TraceV0','loadH5TraceV1','loadH5TraceV2']

from utils import *

