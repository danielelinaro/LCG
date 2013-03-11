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
    )
