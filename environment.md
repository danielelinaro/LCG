---
layout: modernist
title: Environment variables
---

# LCG environment variables

The environment variables exported in lcg-env.sh provide necessary
defaults to LCG and should be tailored to your system.
In particular, the file exports the following variables:

- COMEDI_DEVICE The path to the device file from which data is read.
- AI_CONVERSION_FACTOR_CC The conversion factor to be used for the analog input, in current clamp mode.
- AO_CONVERSION_FACTOR_CC The conversion factor to be used for the analog output, in current clamp mode.
- AI_CONVERSION_FACTOR_VC The conversion factor to be used for the analog input, in voltage clamp mode.
- AO_CONVERSION_FACTOR_VC The conversion factor to be used for the analog output, in current clamp mode.
- RANGE The range of the output to the analog card.
- AI_SUBDEVICE The analog input subdevice on the acquisition card.
- AI_CHANNEL_CC The default channel used for analog input in current clamp moode.
- AI_CHANNEL_VC The default channel used for analog input in voltage clamp moode.
- AO_SUBDEVICE The analog output subdevice on the acquisition card.
- AO_CHANNEL_CC The default channel used for analog output in current clamp mode.
- AO_CHANNEL_VC The default channel used for analog output in voltage clamp mode.
- AI_UNITS_CC The units for the analog input, in current clamp mode.
- AO_UNITS_CC The units for the analog output, in current clamp mode.
- AI_UNITS_VC The units for the analog input, in voltage clamp mode.
- AO_UNITS_VC The units for the analog output, in voltage clamp mode.
- DIGITAL_SUBDEVICE The digital subdevice on the acquisition card.
- DIGITAL_CHANNEL The default digital channel.
- SAMPLING_RATE The default sampling rate of the acquisition.
- GROUND_REFERENCE The ground reference of the acquisition card. At present,
  Ground-Referenced Single Ended (GRSE) and Non-Referenced Single Ended (NRSE) are supported.
- LCG_REALTIME This variable tells whether the system
  should preferentially use the real-time kernel if it is available or
  not. The main advantage in using the real-time kernel (also for open
  loop experiments) is that it provides synchronous input and output,
  in contrast to the non-real-time mode, where input and output are
  asynchronously managed by the DAQ board.
- LCG_RESET_OUTPUT This variable tells whether the
  output of the DAQ board should automatically be reset to zero every
  time a trial ends. This is particularly useful when interrupting the
  program (with Ctrl-C for example) and is valid only for current
  clamp experiments.
- KERNEL DUR The duration of the white noise injection used in the computation
  of the kernel used for the Active Electrode Compensation. The default values is 10 s.
- KERNEL STD The standard deviation (in pA) of the noisy current injected for the computation
  of the kernel. The default value of 200 pA is suitable for cortical cells with input
  resistance in the range 50 − 150 M&#8486;. As a rule of thumb, decrease this value if
  you are dealing with small cells that present an input resistance in excess of 200 M&#8486;.

Most of the previous values depend on how your amplifier is configured
and on how it is wired to the acquisition card. It is also important to
note that the conversion factors and the units provided in
.lcg-env.sh are meaningful when only one input and output
channel are present. In all other cases, input/output conversion
factors will have to be specified either in the configuration file or
when invoking a script. LCG provides a script that helps the
user in finding the correct values for the conversion factors used on
his/her system. To use it, turn on the amplifier and connect it to the
board as you would during an experiment and run the following commands:

         $ comedi_calibrate
         $ lcg-find-conversion-factors --CC-channels 0,0 --VC-channels 1,1

where the --CC-channels and --VC-channels
options specify the input and output channels to use in current or
voltage clamp mode, respectively, and should reflect the values
used in the system that is being configured.
The script will ask the user a few questions and then update the
values of the conversion factors in the .lcg-env.sh file:
due to rounding errors, however, these values might have to be rounded
by the user afterwards by editing manually the .lcg-env.sh file.

