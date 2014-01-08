/*=========================================================================
 *
 *   Program:     lcg
 *   Filename:    common.h
 *
 *   Copyright (C) 2012,2013,2014 Daniele Linaro
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *   
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *   
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *=========================================================================*/

#ifndef COMMON_H
#define COMMON_H

#if defined(HAVE_CONFIG_H)
#include "config.h"     // defines VERSION
#else
#define VERSION "0.9.1"
#endif

/*!
 * \file common.h
 * \brief Header file with common definitions
 */

/*!
 * \mainpage A Dynamic Clamp library and toolbox (lcg)
 *
 * \author Daniele Linaro <daniele.linaro@ua.ac.be>
 * \author Joao Couto <joao@tnb.ua.ac.be>
 * \version 0.9.1
 * \date 2012,2013
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
 * \namespace lcg
 * \brief Main namespace that contains all classes and the utility functions of the toolbox.
 */
namespace lcg {

}

#include <limits>

#define LOGFILE                 "/tmp/lcg.txt"
#define TRIM_ANALOG_OUTPUT

// WHY is this enumeration here???
enum {
        PLUS_MINUS_TEN = 0,
        PLUS_MINUS_FIVE,
        PLUS_MINUS_ONE,
        PLUS_MINUS_ZERO_POINT_TWO
};

#define INFINITE     std::numeric_limits<double>::infinity() 
#define NOT_A_NUMBER std::numeric_limits<double>::quiet_NaN() 
#define ONE_OVER_SIX 0.16666666667

#define COMMENT_MAXLEN  1024
#define FILENAME_MAXLEN  128

#endif

