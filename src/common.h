#ifndef COMMON_H
#define COMMON_H

/*!
 * \file common.h
 * \brief Header file with common definitions
 */

/*!
 * \mainpage A Dynamic Clamp library and toolbox (dynclamp)
 *
 * \author Daniele Linaro <daniele.linaro@ua.ac.be>
 * \author Joao Couto <joao@tnb.ua.ac.be>
 * \version 0.1
 * \date 2012
 * 
 * \section con Contents:
 *
 * -# \ref intro "Introduction"
 * -# \ref install "Installation"
 * -# \ref examples "Examples of use"
 * -# \ref links "Useful Links"
 *
 * \page intro Introduction
 *
 * \page install Installation
 *
 * \page examples Examples of use
 *
 * \page links Useful links
 * This project uses the following external libraries:<br/>
 * - <a href="http://hdfgroup.org/HDF5">HDF5</a> for saving data.
 * - <a href="http://www.boost.org">Boost</a> for a lot of stuff.
 * - <a href="http://www.comedi.org">Comedi</a> for data acquisition.
 *
 */

/*!
 * \namespace dynclamp
 * \brief Main namespace that contains all classes and the utility functions of the toolbox.
 */
namespace dynclamp {

}

#define LOGFILE                 "/tmp/dynclamp.txt"
//#define ASYNCHRONOUS_INPUT
#define TRIM_ANALOGOUTPUT

// WHY is this enumeration here???
enum {
        PLUS_MINUS_TEN = 0,
        PLUS_MINUS_FIVE,
        PLUS_MINUS_ONE,
        PLUS_MINUS_ZERO_POINT_TWO
};

#endif

