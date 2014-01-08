/*=========================================================================
 *
 *   Program:     lcg
 *   Filename:    error_msgs.h
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

 error_msgs.h   : library containing support functions to report errors and warnings to
                  the standard error. (header file)

***********************************************************************************************/

#ifndef ERROR_MSGS_H
#define ERROR_MSGS_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

void msg(char*, int);
void error(char*, int);
void warning(char *, int);

#ifdef __cplusplus
}
#endif

#endif

