/*=========================================================================
 *
 *   Program:     lcg
 *   Filename:    stimgen_common.h
 *
 *   Copyright (C) 2004,2005 Michele Giugliano
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

#ifndef STIMGEN_COMMON_H
#define STIMGEN_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#define STIMGEN_SOFTWARE            "CREATE_STIMULUS"
#define STIMGEN_VERSION             2012

#define TWOPI 6.283185307179586476925286766559   // Mathematical constant := 2 * PI.
#define POSPART(m) (((m) > 0) ? m : 0.)

#define DURATION 0
#define CODE     1
#define P1       2
#define P2       3
#define P3       4
#define P4       5
#define P5       6
#define FIXSEED  7
#define MYSEED   8
#define SUBCODE  9
#define PREC_OP 10
#define EXPON   11

#define DC_WAVE         1
#define ORNUHL_WAVE     2
#define SINE_WAVE       3
#define SQUARE_WAVE     4
#define SAW_WAVE        5
#define SWEEP_WAVE      6
#define RAMP_WAVE       7
#define POISSON1_WAVE   8
#define POISSON2_WAVE   9
#define BIPOLAR_WAVE    10
#define UNIF_NOISE      11
#define ALPHA_FUN       12

#define SUMMATION       1
#define MULTIPLICATION  2
#define SUBTRACTION     3
#define DIVISION        4

typedef unsigned int uint;                   // Type definition for my convenience.

#ifdef __cplusplus
}
#endif

#endif

