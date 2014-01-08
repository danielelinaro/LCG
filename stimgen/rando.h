/*=========================================================================
 *
 *   Program:     lcg
 *   Filename:    rando.h
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

#ifndef RANDO_H
#define RANDO_H

#ifdef __cplusplus
extern "C" {
#endif

/* 
   float drand49():      returns a uniformly distributed (pseudo)random (float) number between 0.0 and 1.0 
   float gauss():        returns a Gauss-distributed (pseudo)random (float) number with zero mean and unitary variance 
   float srand49(long):  inits the 'seed' and returns a.... between 0.0 and 1.0
(from Numerical Recipes) 
   long mysrand49(long): inits the 'seed' and returns the *previous* seed (custom made!)
*/
   
long mysrand49(long seed);
float srand49(long seed);
float drand49(void);
float gauss(void);
int srand10(long seed);
int drand10(void);

#ifdef __cplusplus
}
#endif

#endif

