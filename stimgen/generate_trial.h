/*=========================================================================
 *
 *   Program:     lcg
 *   Filename:    generate_trial.h
 *
 *   Copyright (C) 2004-2009 Michele Giugliano
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

/***********************************************************************************************

 Antwerp, 11/7/2009
 Bern, 22/1/2004 - Michele Giugliano, PhD

***********************************************************************************************/

#ifndef GENERATE_TRIAL_H
#define GENERATE_TRIAL_H

#include <math.h>
#include <time.h>
#include <stdio.h>

#include "error_msgs.h"
#include "file_parsing.h"
#include "waveforms.h"
#include "stimgen_common.h"

#ifdef __cplusplus
extern "C" {
#endif

int generate_trial(const char *, int , int , char *, double **, uint *, double, double);	// function prototype

#ifdef __cplusplus
}
#endif

#endif

