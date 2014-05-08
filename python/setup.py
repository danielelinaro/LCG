#!/usr/bin/env python

import os
from setuptools import setup
#from distutils.core import setup

def read(fname):
    return open(os.path.join(os.path.dirname(__file__), fname)).read()

setup(
    name = 'lcg',
    version = '0.9',
    author = 'Daniele Linaro and Joao Couto',
    author_email = 'danielelinaro@gmail.com',
    description = ('lcg module for Python.'),
    long_description = read('README'),
    license = 'GPL',
    url = 'http://www.tnb.ua.ac.be',
    packages = ['lcg','aec'],
    entry_points = {
        'console_scripts': [
            'disynaptic_microcircuit = lcg.disynaptic_microcircuit:main',
            'izhikevich_network = lcg.izhikevich_network:main',
            'prc_protocol = lcg.prc_protocol:main',
            'reliability_disynaptic = lcg.reliability_disynaptic:main',

            'lcg-ap = lcg.ap:main',
            'lcg-correlations = lcg.correlations:main',
            'lcg-cv = lcg.cv:main',
            'lcg-ecode = lcg.ecode:main',
            'lcg-extracellular = lcg.extracellular_protocol:main',
            'lcg-extract-kernels = lcg.extract_kernels:main',
            'lcg-fclamp = lcg.fclamp:main',
            'lcg-fi = lcg.fi_pid:main',
            'lcg-fopt = lcg.fopt:main',
            'lcg-kernel = lcg.kernel:main',
            'lcg-ltpltd = lcg.ltp_ltd:main',
            'lcg-ou = lcg.ou_protocol:main',
            'lcg-pspopt = lcg.psp_opt:main',
            'lcg-pulse = lcg.pulse:main',
            'lcg-pulses = lcg.pulses:main',
            'lcg-pulses-with-level = lcg.repetitive_pulses_with_level:main',
            'lcg-ramp = lcg.ramp:main',
            'lcg-rate-steps = lcg.rate_steps:main',
            'lcg-rcwrite = lcg.rc_write:main',
            'lcg-Rin = lcg.Rin:main',
            'lcg-sinusoids = lcg.sinusoids:main',
            'lcg-spontaneous = lcg.spontaneous:main',
            'lcg-steps = lcg.step:main',
            'lcg-stimgen = lcg.stimgen:main',
            'lcg-stimulus = lcg.stimulus:main',
            'lcg-tau = lcg.tau:main',
            'lcg-vi = lcg.vi:main',
            'lcg-plot = lcg.plot_file:main',
            'lcg-zap = lcg.zap:main',
            'lcg-find-conversion-factors = lcg.find_conversion_factors:main'
            ]
        }
    )
