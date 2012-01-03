/***********************************************************************************************

 Antwerp, 10/07/2005 - Michele Giugliano, PhD
 Bern, 22/1/2004

 error_msgs.h   : library containing support functions to report errors and warnings to
                  the standard error. (header file)

***********************************************************************************************/

#ifndef ERROR_MSGS_H
#define ERROR_MSGS_H

#include <stdio.h>

void msg(char*, int);
void error(char*, int);
void warning(char *, int);

#endif

