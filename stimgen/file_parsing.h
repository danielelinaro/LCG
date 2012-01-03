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

#define MAXCOLS             15      // maximal number of columns in the input data vector
#define MAXROWS             100     // maximal number of lines 

int extract(double *,char *);
int readmatrix(char *, double **, INT *, INT *);

#endif

