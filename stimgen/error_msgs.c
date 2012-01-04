/***********************************************************************************************
 
 Antwerp, 10/07/2005 - Michele Giugliano, PhD
 Bern, 22/1/2004
 
 error_msgs.c   : library containing support functions to report errors and warnings to
                  the standard error.

***********************************************************************************************/

#include "error_msgs.h"
#include "stimgen_common.h"

void msg(char *mytext, int verbose) {
 char mystring[500];
 if (verbose) {
 sprintf(mystring, "[%s - %d]: %s\n", SOFTWARE, VERSION, mytext);
 fprintf(stderr, "%s", mystring); fflush(NULL);
 }
 return;
} // end msg()


void error(char *mytext, int verbose) {
 char mystring[500];
 if (verbose) {
 sprintf(mystring, "(Error)[%s - %d]: %s\n", SOFTWARE, VERSION, mytext);
 fprintf(stderr, "%s", mystring); fflush(NULL);
 }
 return;
} // end error()


void warning(char *mytext, int verbose) {
 char mystring[500];
 if (verbose) {
 sprintf(mystring, "(Warning)[%s - %d]: %s\n", SOFTWARE, VERSION, mytext);
 fprintf(stderr, "%s", mystring); fflush(NULL);
 }
 return;
} // end warning()
