---
layout: modernist
title: Installation
---

# LCG installation instructions
This page provides step-by-step instruction on how to install LCG on your system.

The primary application of LCG is to perform electrophysiology experiments.
Albeit not necessary, as it will be described in the following, it is however
recommended to install lcg on a Linux machine with a data acquisition card
and a working installation of the Comedi library. A real-time kernel is
required only to perform closed-loop or dynamic clamp experiments.
It is also possible to install lcg on any UNIX compatible operating system
(Linux or Mac OS X, for instance) without Comedi and real-time capabilities.
This approach can be useful to familiarize with the program and to develop
new experiments. A detailed description of how to do this is given at the end
of this page.

In the following sections, we describe how to set up a Linux machine with a
real time kernel and Comedi and how to install LCG. We assume that you have
a working Linux installation: if that is not the case, refer to the documentation
of one of the multiple distributions available. In our laboratory we use
Debian and therefore the following instructions will be based on this distribution.

## Configuration of the Linux machine
LCG can be installed on a variety of Linux distributions. Here, we will refer to
the stable version of Debian at the time of this writing (Squeeze 6.0).

### Patching and installing the real time kernel
If you don’t want to use the real-time capabilities of LCG you can skip this paragraph.

This is potentialy the most difficult part of the installation. In order to achieve
nanosecond precision, LCG requires a kernel with real time capabilities. Both RTAI
and PREEMPT RT can be used. The latter is advisable since RTAI does not include
support for the latest kernels, which might work better with the most recent hardware.

You may need to install tools to build the kernel. In Debian you can type, as root:

        # apt-get update  
        # apt-get install build-essential binutils-dev libelf-dev libncurses5
          libncurses5-dev git-core make gcc subversion libc6 libc6-dev
          automake libtool bison flex autoconf flex libgsl0-dev

At the moment, LCG supports two real-time kernels, [RTAI](https://www.rtai.org/) and
[PREEMPT_RT](https://rt.wiki.kernel.org/index.php/Main_Page). In our laboratory, we use
PREEMPT_RT, since RTAI does not include support for the latest kernels, which might work
better with the most recent hardware.

The following procedure should have you up and running:

1. **Check which kernels and patches are available**.  
   The realtime patch is best installed on a plain vanilla kernel.
   Go to the the [real-time page on kernel.org](https://www.kernel.org/pub/linux/kernel/projects/rt/)
   and check which patches are available for the kernel that you wish to install.

2. **Download the kernel and the realtime patch**.  
   Assuming that you want to perform this operation in /usr/src, you can type (as root):

        # cd /usr/src
        # wget https://www.kernel.org/pub/linux/kernel/v3.x/linux-3.8.4.tar.bz2
        # wget http://www.kernel.org/pub/linux/kernel/projects/rt/3.8/patch-3.8.4-rt2.patch.bz2

3. **Decompress and patch the kernel**.  
   Patching the kernel will add real- time support to the kernel you have downloaded.

        # tar xvf linux -3.8.4.tar.bz2
        # ln -s linux-3.8.4 linux
        # cd linux
        # bzcat ../patch-3.8.4-rt2.patch.bz2 | patch -p1

4. **Configure the kernel**.  
   The easiest way to do this is to use the configuration file from a kernel that was
   previously installed in your system or from a similar installation/system.

        # cp /boot/config-‘uname -r‘ .config
        # make oldconfig
        # make menuconfig

   This will evoke a user interface to configure the kernel options.
   The following options are important for a correct function of the real-time system:  
      + Set the Preemption model to Fully Preemptible Kernel (RT) in the Processor type and features.
      + Disable CPU Frequency scaling under Power Management and ACPI options.
      + Disable Check for stack overflow and the options under Tracers in Kernel hacking.
      + Additionally you should also disable the Comedi drivers under the Staging drivers of the Device drivers menu.

5. Compile and install the kernel This step might from several minutes to hours, depending on the size of the kernel.

        # make && make modules && make modules_install && make install

   When the installation is complete you will need to update the boot loader.

        # cd /boot
        # update-initramfs -c -k 3.8.4-rt2
        # update-grub

   Check that grub has been updated. You should see an entry with the name of the newly installed kernel in /etc/grub/menu.lst.
   After confirmation reboot into your new kernel.

### Installation of LCG dependencies

LCG requires the following libraries:

1. [**BOOST**](http://www.boost.org/).  
   This library is used for parsing XML configuration files. The website provides extensive
   documentation on how to install the library, but generally it is sufficient to type (for example
   with version 1.53.0):

        $ wget http://sourceforge.net/projects/boost/files/boost/1.53.0/boost_1_53_0.tar.bz2
        $ tar --bzip2 -xvf boost_1_53_0.tar.bz2
        $ cd boost_1_53_0
        $ ./bootstrap.sh --prefix=/usr/local
        # ./b2 install

2. [**Comedi**](http://www.comedi.org).  
   If you are going to use a data acquisition board (DAQ) to acquire signals you need to install the drivers
   that LCG uses to communicate with the data acquisition card.  
   To download and install the kernel modules, type:

        $ git clone git://comedi.org/git/comedi/comedi.git
        $ cd comedi
        $ ./autogen.sh
        $ ./configure --with-linuxdir=/usr/src/linux
        $ make
        # make install
        # depmod -a

   This creates a device, usually called /dev/comedi0, that represents the interface to the DAQ card.  
   To download and install comedilib, the user space interface to the kernel module, type:

        $ git clone git://comedi.org/git/comedi/comedilib.git
        $ cd comedilib
        $ ./autogen.sh
        $ ./configure --prefix=/usr/local
        $ make
        # make install

   Finally, to download and install comedi_calibrate, the tools to calibrate the DAQ cards, type,
   after having installed BOOST:

        $ git clone git://comedi.org/git/comedi/comedi_calibrate.git
        $ cd comedi_calibrate
        $ ./autogen.sh
        $ ./configure --prefix=/usr/local
        $ make
        # make install

3. [**HDF5**](http://www.hdfgroup.org/HDF5/).
   This library is used for data storage and management with high performance and efficiency. These libraries can be installed
   from source in the following way:

        $ tar xzf hdf5-1.8.9.tar.bz2
        $ cd hdf5-1.8.9
        $ ./configure --prefix=/usr/local --enable-shared
        $ make
        # make install

   Alternatively, it is possible to use a package manager, such as Debian’s apt-get:

        # apt-get install libhdf5-serial-1.8.4 libhdf5-serial-dev

## Installation of LCG

To install LCG, start by getting it from the [GitHub repository](https://github.com/danielelinaro/dynclamp).
We recommend to install LCG in a local folder, so that it is easier to have
multiple installations for different users.

        $ mkdir $HOME/local/src
        $ cd $HOME/local/src
        $ git clone https://github.com/danielelinaro/dynclamp.git lcg
        $ cd dynclamp 
        $ autoreconf -i 
        $ ./configure --prefix=$HOME/local 
        $ make 
        # make install 

Alternatively, LCG can be installed to other locations by changing the prefix.

### Installation of the Python bindings

Some features such as kernel estimation for the Active Electrode Compensation require the installation
of an additional Python module. This requires you to install [Numpy](http://www.numpy.org/), [Scipy](http://www.scipy.org/),
[Matplotlib](http://matplotlib.org/), [PyTables](http://www.pytables.org) and [lxml](http://lxml.de). This can
be done either from source or, if you are using Debian, by issuing the following command:

        # apt-get install python-numpy python-scipy python-matplotlib python-tables python-lxml

In the LCG root directory, type:

        $ cd python
        $ python setup.py build
        $ python setup.py install --prefix=$HOME/local

This will install the module in the directory $HOME/local/lib/python2.6/site-packages,
which needs to be added to your PYTHONPATH environment variable, by adding the
following line to your .bashrc file:

        export PYTHONPATH=$PYTHONPATH:$HOME/local/lib/python2.6/site-packages

## Final configuration

### Configuration of the real time system

To have a working real time system, two additional steps are required:

1. Grant read/write permissions to the Comedi device to all users: to permanently change the permissions of the device file used by
Comedi, in Debian you should create a file called /etc/udev/rules.d/99-comedi.rules with the following line:

        KERNEL=="comedi0", MODE="0666"

2. Grant permission to a group of users to run programs in real time: first create a realtime
group and add to it the users that will use LCG.

        # groupadd realtime
        # usermod -a -G realtime USER

   Then, add the following lines to the file /etc/security/limits.conf:

        @realtime       -       rtprio  99
        @realtime       -       memlock unlimited

   The changes will take effect once you log out and log back in.

### Configuration of the user environment

To set up the user environment so that LCG can function properly, do the following.
First of all, add the directory where lcg binaries where installed to your path.
To do this, add the following line to your .bashrc file:

        export PATH=$PATH:$HOME/local/bin

After this, copy the files lcg-env.sh and lcg-completion.bash from lcg base directory to your home directory:

        $ cp lcg-env.sh ~/.lcg-env.sh
        $ cp lcg-completion.bash ~/.lcg-completion.bash

Source them any time you log in by adding the following lines to your .bashrc file:

        source ~/.lcg-env.sh
        source ~/.lcg-completion.bash

The script in lcg-completion.bash provides autocomplete capabilities to LCG but it is not
required for correct functioning. The environment variables exported in lcg-env.sh, on the
other hand, provide necessary defaults to LCG and should be tailored to your system.
In particular, the file exports the following variables:

- COMEDI DEVICE The path to the device file from which data is read.
- AI CONVERSION FACTOR CC The conversion factor to be used for the analog input, in current clamp mode.
- AO CONVERSION FACTOR CC The conversion factor to be used for the analog output, in current clamp mode.
- AI CONVERSION FACTOR VC The conversion factor to be used for the analog input, in voltage clamp mode.
- AO CONVERSION FACTOR VC The conversion factor to be used for the analog output, in current clamp mode.
- RANGE The range of the output to the analog card.
- AI SUBDEVICE The analog input subdevice on the acquisition card.
- AI CHANNEL The default channel used for analog input.
- AO SUBDEVICE The analog output subdevice on the acquisition card.
- AO CHANNEL The default channel used for analog output.
- AI UNITS CC The units for the analog input, in current clamp mode.
- AO UNITS CC The units for the analog output, in current clamp mode.
- AI UNITS VC The units for the analog input, in voltage clamp mode.
- AO UNITS VC The units for the analog output, in voltage clamp mode.
- SAMPLING RATE The default sampling rate of the acquisition.
- GROUND REFERENCE The ground reference of the acquisition card. At present,
  Ground-Referenced Single Ended (GRSE) and Non-Referenced Single Ended (NRSE) are supported.

Most of the previous values depend on how your amplifier is configured and on how it is
wired to the acquisition card. It is also important to note that the conversion factors
and the units provided in .lcg-env.sh are meaningful when only one input and output
channels are present. In all other cases, input/output conversion factors will have to
be specified either in the configuration file or when invoking a script.

## Installation without a realtime kernel

In order to learn how to use LCG, to test and debug configuration files and scripts or to run non-realtime simulations, it is possible
to perform an installation on a non-real-time machine. LCG has been installed successfully and tested both on Linux and Mac OS X.
In this case, it is necessary to install only the BOOST and HDF5 libraries and the system-wide configuration steps described previously
are not necessary. If you are installing lcg on Mac OS X, note that the equivalent of the .bashrc file is called .profile.

