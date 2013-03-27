#!/usr/bin/env python

import os
from setuptools import setup

def read(fname):
    return open(os.path.join(os.path.dirname(__file__), fname)).read()

setup(
    name = 'lcg',
    version = '0.9',
    author = 'Daniele Linaro and Joao Couto',
    author_email = 'daniele.linaro@ua.ac.be',
    description = ('lcg module for Python.'),
    long_description = read('README'),
    license = 'GPL',
    url = 'http://www.tnb.ua.ac.be',
    packages = ['lcg','aec'],
    entry_points = {
        'console_scripts': [
            'extract_kernels = lcg.extract_kernels:main',
            'Rin_protocol = lcg.Rin_protocol:main',
            'cv = lcg.cv:main',
            'disynaptic_microcircuit = lcg.disynaptic_microcircuit:main',
            'extracellular_protocol = lcg.extracellular_protocol:main',
            'f_opt = lcg.f_opt:main',
            'fclamp = lcg.fclamp:main',
            'psp_opt = lcg.psp_opt:main',
            'pulses_protocol = lcg.pulses_protocol:main',
            'reliability_disynaptic = lcg.reliability_disynaptic:main',
            'sinusoids_protocol = lcg.sinusoids_protocol:main',
            'steps_protocol = lcg.steps_protocol:main',
            'ltp_ltd_protocol = lcg.ltp_ltd_protocol:main'
            ]
        }
    )
