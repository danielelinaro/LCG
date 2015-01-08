---
layout: modernist
title: Documentation
---

## LCG documentation
The manual that describes the full capabilities and rationale behind LCG
can be found [here](lcg_manual.pdf). Please note that this is a current
work in progress and therefore some sections are still missing. Also,
bear in mind that the linked version might lag behind the most updated
version of the manual, which will always be available on GitHub. To
retrieve the latest version, checkout the "manual" branch from GitHub
and compile the pdf using the following commands:

        $ cd $HOME
        $ git clone https://github.com/danielelinaro/dynclamp.git lcg
        $ cd lcg
        $ git checkout manual
        $ pdflatex manual.tex

This will produce a manual.pdf file with the most updated version of
the manual. At present, we are still adding content to it: for the
moment, please feel free to contact [Daniele
Linaro](mailto:danielelinaro@gmail.com) for any doubt you might have 
regarding the installation and the operation of LCG.

LCG uses stimulus files to describe arbitrary waveforms: such stimulus files
use a meta-description called STIMGEN format, whose syntax, together with
several examples, is described in [this manual](stimgen_manual.pdf).
