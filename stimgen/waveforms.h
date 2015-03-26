/*=========================================================================
 *
 *   Program:     lcg
 *   Filename:    waveforms.h
 *
 *   Copyright (C) 2003,2004,2005,2013 Michele Giugliano, Daniele Linaro
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

 Antwerp, 10/07/2005 - Michele Giugliano, PhD
 Bern, 23/11/2003
 Bern, 22/1/2004
 
 waveform.h   : 

***********************************************************************************************/

#ifndef WAVEFORMS_H
#define WAVEFORMS_H

#include "stimgen_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
        LINEAR = 0, LOG
} sweep_type;

double how_long_lasts_trial(double **, uint);
int    simple_waveform(double *, double *, uint *, uint, double, double, int);
int    composite_waveform(double **, uint, double *, uint *, uint, double, double, int);

void DC(double, double *, uint *, uint, double, double, double);
void GAUSS(double, double, double *, uint *, uint, double, double, double); 
void ORUHL(double, double, double, double, double *, uint *, uint, double, double, double);
void SINE(double, double, double, double, double *, uint *, uint, double, double, double);
void POISSON_SHOT1(double, double, double, double *, uint *, uint, double, double, double);
void POISSON_SHOT2(double, double, double, double *, uint *, uint, double, double, double);
void BIPOLAR_SHOT(double, double, double, double *, uint *, uint, double, double, double);
void RAMP(double, double *, uint *, uint, double, double, double);      
void SQUARE(double, double, double, double *, uint *, uint, double, double, double);
void SAW(double, double, double, double *, uint *, uint, double, double, double); 
void SWEEP(double, double, double, sweep_type, double *, uint *, uint, double, double, double);
void UNIFNOISE(double, double, double *, uint *, uint, double, double, double); 
void ALPHA(double, double, double, double *, uint *, uint, double, double, double); 
#ifdef __cplusplus
}
#endif

#endif

