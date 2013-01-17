#!/usr/bin/env python

from distutils.core import setup

setup(name='dclamp',
      version='0.9',
      description='dynclamp utilities for Python.',
      author='Daniele Linaro and Joao Couto',
      author_email='daniele@tnb.ua.ac.be',
      url='http://www.tnb.ua.ac.be',
      packages=['dclamp','AEC'],
      scripts=['scripts/f_opt.py', 'scripts/cv.py', 'scripts/psp_opt.py', 'scripts/reliability_disynaptic.py']
     )
