/*=========================================================================
 *
 *   Program:     lcg
 *   Filename:    file_parsing.h
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

/***********************************************************************************************
 
 Antwerp, 10/07/2005 - Michele Giugliano, PhD
 Bern, 22/1/2004
 
 file_parsing.h     : library containing procedures used for input file parsing. (header file)
    
***********************************************************************************************/

#ifndef FILE_PARSING_H
#define FILE_PARSING_H

#include <stdio.h>
#include <stdlib.h>                 // This is required for the 'atof()' function.
#include <string.h>
#include "stimgen_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAXCOLS             15      // maximal number of columns in the input data vector
#define MAXROWS             100     // maximal number of lines 

int extract(double *, const char *);
int readmatrix(const char *, double **, size_t *, size_t *);

#ifdef __cplusplus
}
#endif

#endif

