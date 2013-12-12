#!/usr/bin/env python

import os
from setuptools import setup

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
            'extract_kernels = lcg.extract_kernels:main',
            'Rin_protocol = lcg.Rin_protocol:main',
            'lcg-cv = lcg.cv:main',
            'disynaptic_microcircuit = lcg.disynaptic_microcircuit:main',
            'lcg-extracellular = lcg.extracellular_protocol:main',
            'lcg-fopt = lcg.fopt:main',
            'lcg-fclamp = lcg.fclamp:main',
            'lcg-pspopt = lcg.psp_opt:main',
            'lcg-pulses = lcg.pulses:main',
            'reliability_disynaptic = lcg.reliability_disynaptic:main',
            'lcg-sinusoids = lcg.sinusoids:main',
            'lcg-steps = lcg.step:main',
            'lcg-ltpltd = lcg.ltp_ltd:main',
            'prc_protocol = lcg.prc_protocol:main',
            'izhikevich_network = lcg.izhikevich_network:main',
            'lcg-correlations = lcg.correlations:main',
            'lcg-ou = lcg.ou_protocol:main',
            'lcg-fi = lcg.fi_pid:main',
            'lcg-ap = lcg.ap:main',
            'lcg-tau = lcg.tau:main',
            'lcg-pulse = lcg.pulse:main',
            'lcg-ramp = lcg.ramp:main',
            'lcg-vi = lcg.vi:main',
            'lcg-rcwrite = lcg.rc_write:main',
            'lcg-kernel = lcg.kernel:main',
            'lcg-rate-steps = lcg.rate_steps:main',
            'lcg-pulses-with-level = lcg.repetitive_pulses_with_level:main',
            'lcg-stimgen = lcg.stimgen_interface:main',
            'lcg-spontaneous = lcg.spontaneous:main'
            ]
        }
    )
