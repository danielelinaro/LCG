---
layout: modernist
title: Installation
---

## LCG installation instruction
This page provides step-by-step instruction on how to install LCG on your system.

### Patch and compile the kernel
The first step is to have a working Linux system with a real-time kernel.  
In the following, we shall refer to a [Debian distribution](http://www.debian.org),
but in principle any other distribution that allows you to recompile the kernel
should work.  
In Debian, you can install all the tools required to recompile the kernel by issuing the
following command (as root):  

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

### Install LCG dependencies

LCG requires the following libraries:

1. [**BOOST**](http://www.boost.org/).  
   This library is used for several things, among which
   are multithreading, argument parsing, XML parsing and a few others. The website provides extensive
   documentation on how to install the library, but generally it is sufficient to type (for example
   with version 1.53.0):

        $ wget http://sourceforge.net/projects/boost/files/boost/1.53.0/boost_1_53_0.tar.bz2
        $ tar --bzip2 -xvf boost_1_53_0.tar.bz2
        $ cd boost_1_53_0.tar.bz2
        $ ./bootstrap.sh
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
   from source or, on Debian, by typing:

        # apt-get install libhdf5-serial-1.8.4 libhdf5-serial-dev

### Compile and install the LCG library and applications

Clone LCG from the [GitHub repository](https://github.com/danielelinaro/dynclamp). You can install it system-wide with the following
commands:

        $ git clone https://github.com/danielelinaro/dynclamp.git
        $ cd dynclamp
        $ ./autoreconf -i
        $ ./configure --prefix=/usr/local
        $ make
        # make install

Alternatively, LCG can be installed to other locations by changing the prefix.

### Install the Python module and scripts

Some features such as kernel estimation for the Active Electrode Compensation require the installation
of an additional Python module. This requires you to install [Numpy](http://www.numpy.org/), [Scipy](http://www.scipy.org/),
[Matplotlib](http://matplotlib.org/), [PyTables](http://www.pytables.org) and [lxml](http://lxml.de). This can
be done either from source or, if you are using Debian, by issuing the following command:

        # apt-get install python-numpy python-scipy python-matplotlib python-tables python-lxml

In the LCG root directory, type:

        $ cd python
        $ python setup.py build
        # python setup.py install

Alternatively you can install it in another directory, by specifying a different prefix option in the last command.
In this case, also remember to add the specified folder to the PYTHONPATH environment variable in your .bashrc or .bash_profile file.

## Configuration of the system

To have a working system, two additional steps are required:

1. Grant read/write permissions to the Comedi device to all users: to permanently change the permissions of the device file used by
Comedi, in Debian you should create a file called /etc/udev/rules.d/99-comedi.rules with the following line:

        KERNEL=="comedi0", MODE="0666"

2. Grant permission to a group of users to run programs in real-time: first create a realtime group and add to it the users that will use LCG.

        # groupadd realtime
        # usermod -a -G realtime USER

   Then, add the following lines to the file /etc/security/limits.conf:

        @realtime       -       rtprio  99
        @realtime       -       memlock unlimited

   The changes will take effect once you log out and log back in.

## Installation without a realtime kernel

In order to learn how to use LCG, to test and debug configuration files and scripts or to run non-realtime simulations, it is possible
to perform an installation on a non-real-time machine. LCG has been installed successfully and tested both on Linux and Mac OS X.
In this case, it is necessary to install only the BOOST and HDF5 libraries and the system-wide configuration steps described previously
are not necessary.

