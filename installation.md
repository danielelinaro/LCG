---
layout: modernist
title: Installation
---

# LCG installation instructions
This page provides step-by-step instructions on how to install LCG on your system.

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

<b>NOTES</b>: throughout the following instructions, commands starting
with a hash sign (<b>#</b>) need to be run as root, whereas commands starting
with a dollar sign (<b>$</b>) may be run as a normal
user. Additionally, the environment variable <b>$HOME</b> indicates the
home directory of the user that is issuing a specific command: for
instance, if the user is called john, $HOME will be equal to /home/john.

If you find any errors or omissions in the following instructions,
please contact [Daniele Linaro](mailto:danielelinaro@gmail.com) or [João Couto](mailto:jpcouto@gmail.com).

## Configuration of the Linux machine
LCG can be installed on a variety of Linux distributions. Here, we will refer to
the stable version of Debian at the time of this writing (Squeeze 6.0).

### Patching and installing the real time kernel
If you don’t want to use the real-time capabilities of LCG you can skip this paragraph.

This is potentialy the most difficult part of the installation. In order to achieve
nanosecond precision, LCG requires a kernel with real time capabilities. Both [RTAI](https://www.rtai.org)
and [PREEMPT RT](https://rt.wiki.kernel.org) can be used. The latter is advisable since RTAI does not include
support for the latest kernels, which might work better with the most recent hardware.

You may need to install tools to build the kernel. In Debian you can type, as root:

        # apt-get update  
        # apt-get install build-essential binutils-dev libelf-dev libncurses5
          libncurses5-dev git-core make gcc subversion libc6 libc6-dev
          automake libtool bison flex autoconf libgsl0-dev

At the moment, LCG supports two real-time kernels, [RTAI](https://www.rtai.org/) and
[PREEMPT_RT](https://rt.wiki.kernel.org/index.php/Main_Page). In our laboratory, we use
PREEMPT_RT, since RTAI does not include support for the latest kernels, which might work
better with the most recent hardware.

The following procedure should have you up and running:

1. **Check which kernels and patches are available**.  
   The realtime patch is best installed on a plain vanilla kernel.
   Go to the the [real-time page on kernel.org](https://www.kernel.org/pub/linux/kernel/projects/rt/)
   and check which patches are available for the kernel that you wish
   to install. In the following, we will describe how to patch and
   compile the 3.8.4 version of the Linux kernel.

2. **Download the kernel and the realtime patch**.  
   Assuming that you want to perform this operation in /usr/src, you can type (as root):

        # cd /usr/src
        # wget https://www.kernel.org/pub/linux/kernel/v3.x/linux-3.8.4.tar.bz2
        # wget http://www.kernel.org/pub/linux/kernel/projects/rt/3.8/patch-3.8.4-rt2.patch.bz2

3. **Decompress and patch the kernel**.  
   Patching the kernel will add real- time support to the kernel you have downloaded.

        # tar xjf linux-3.8.4.tar.bz2
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
   The following options are important for a correct function of the
   real-time system:

      + Set the <b>Preemption model</b> to <b>Fully Preemptible Kernel (RT)</b> in the <b>Processor type and features</b>.
      + Disable <b>CPU Frequency scaling</b> under <b>Power Management and ACPI options</b>.
      + Disable <b>Check for stack overflow</b> and all options under <b>Tracers</b> in <b>Kernel hacking</b>.
      + Additionally you should also disable the Comedi drivers under the <b>Staging drivers</b> of the <b>Device drivers</b> menu.

5. Compile and install the kernel. This step might take from several minutes to hours, depending on the size of the kernel.

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
   The BOOST C++ library is used for parsing XML files.
   BOOST can be installed by using the system's package manager: on Debian, enter the following
   command as root:

        # apt-get install libboost-dev

2. [**HDF5**](http://www.hdfgroup.org/HDF5/).  
   The HDF Group designed a set of libraries for data storage and
   management with high performance, efficiency and high
   volume/complexity in mind. LCG uses this library to save binary
   data files. HDF5 can be installed by using the system's package
   manager: on Debian, enter the following command as root:

        # apt-get install hdf5-tools hdf5-serial-dev

3. [**Comedi**](http://www.comedi.org).  
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

## Installation of LCG

To install LCG, start by getting it from the [GitHub repository](https://github.com/danielelinaro/dynclamp).
We recommend installing LCG in a local folder, so that it is easier to have
multiple installations for different users.

        $ mkdir -p $HOME/local/src
        $ cd $HOME/local/src
        $ git clone https://github.com/danielelinaro/dynclamp.git lcg
        $ cd lcg 
        $ autoreconf -i 
        $ ./configure --prefix=$HOME/local 
        $ make 
        $ make install 

Alternatively, LCG can be installed to other locations by changing the prefix.

### Installation of the Python bindings

Some features such as kernel estimation for the Active Electrode Compensation require the installation
of an additional Python module. This requires you to install [Numpy](http://www.numpy.org/), [Scipy](http://www.scipy.org/),
[Matplotlib](http://matplotlib.org/), [PyTables](http://www.pytables.org) and [lxml](http://lxml.de). This can
be done either from source or, if you are using Debian, by issuing the following command:

        # apt-get install python-setuptools python-numpy python-scipy python-matplotlib python-tables python-lxml

In the LCG root directory, type:

        $ export PYTHONPATH=$PYTHONPATH:$HOME/local/lib/python2.7/site-packages
        $ cd python
        $ python setup.py build
        $ python setup.py install --prefix=$HOME/local

In case you are using a different version of Python (2.6, for instance), change
the directory accordingly (i.e., to $HOME/local/lib/python2.6/site-packages).

This will install the module in the directory
$HOME/local/lib/python2.7/site-packages, which needs to be permanently
added to your PYTHONPATH environment variable, by adding the previous
export line to your $HOME/.bashrc file.   
This can be accomplished with the following command:

        $ echo 'export PYTHONPATH=$PYTHONPATH:$HOME/local/lib/python2.7/site-packages' >> $HOME/.bashrc

Again, change the directory accordingly if you are using a different Python version.
You can test whether the installation was successful by issuing the following commands:

        $ cd $HOME
        $ python
        >>> import lcg

If the last command produces no error, the installation was successfull.

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

with the command

        $ echo 'export PATH=$PATH:$HOME/local/bin' >> $HOME/.bashrc

After this, copy the files lcg-env.sh and lcg-completion.bash from lcg base directory to your home directory:

        $ cp lcg-env.sh ~/.lcg-env.sh
        $ cp lcg-completion.bash ~/.lcg-completion.bash

Source them any time you log in by adding the following lines to your .bashrc file:

        source ~/.lcg-env.sh
        source ~/.lcg-completion.bash

with the commands

        $ echo 'source ~/.lcg-env.sh' >> $HOME/.bashrc
        $ echo 'source ~/.lcg-completion.bash' >> $HOME/.bashrc

The script in lcg-completion.bash provides autocomplete capabilities to LCG but it is not
required for correct functioning.

For a detailed description of the environment variables used by LCG, read [this page](environment.html).

## Installation without a realtime kernel

In order to learn how to use LCG, to test and debug configuration files and scripts or to run non-realtime simulations, it is possible
to perform an installation on a non-real-time machine. LCG has been installed successfully and tested both on Linux and Mac OS X.
In this case, it is necessary to install only the BOOST and HDF5 libraries and the system-wide configuration steps described previously
are not necessary. If you are installing lcg on Mac OS X, note that the equivalent of the .bashrc file is called .profile.

