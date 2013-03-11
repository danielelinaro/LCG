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

### the scripts
import Rin_protocol
import cv
import disynaptic_microcircuit
import extracellular_protocol
import f_opt
import fclamp
import psp_opt
import pulses_protocol
import reliability_disynaptic
import sinusoids_protocol
import steps_protocol

