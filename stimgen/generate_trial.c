/***********************************************************************************************

 Antwerp, 24/7/2009 - Michele Giugliano, PhD
 Bern, 22/1/2004 - Michele Giugliano, PhD and Maura Arsiero, PhD

***********************************************************************************************/

#include "generate_trial.h"

int generate_trial(const char *filename, int verbose, int output_on_file, char *outfilename, double **output, uint *index, double srate, double dt)
{
char mytext[500];
double **parsed_data;
double T;                           // Total duration [s].
uint Ni;                             // Partial duration [points].
uint N;                              // Size of the output waveform (i.e. N = T / dt).
FILE *fp;

size_t cols, nlines;
uint  current_line;
uint i, return_code;

 //--------------------------------------------------------------------------------------
 // Data structure containing the input file is defined and created here. 

 parsed_data = (double **) calloc(MAXROWS, sizeof(double *));
 if (parsed_data == NULL) { error("Unable to allocate memory for <parsed_data> !", verbose);  return -1; } 
 for (i=0; i<MAXROWS; i++)  { 
  parsed_data[i] = (double *) calloc(MAXCOLS, sizeof(double)); 
  if (parsed_data[i] == NULL) { error("Unable to allocate memory for <parsed_data[i]> !", verbose);  return -1; } 
 }
 //--------------------------------------------------------------------------------------

 //--------------------------------------------------------------------------------------
 // The input-file parsing routine is invoked here..

 if (readmatrix(filename, parsed_data, &nlines, &cols) == -1) {
  error("impossible to proceed.", verbose);  return -1; }
    
  sprintf(mytext,"[%s] acquired correctly: %d lines, %d columns", filename, (int) nlines, (int) cols); msg(mytext, verbose);
  if (nlines == 0) { warning("No stimulus to be parsed: file empty!", verbose);  return -1; }

/*
  printf("The parsed file was:\n"); 
  for (i=0; i<nlines; i++) {
   for (j=0; j<cols+2; j++)
    printf("%f ", parsed_data[i][j]); 
   printf("\n"); 
   } // end for()
*/
 //--------------------------------------------------------------------------------------


 //--------------------------------------------------------------------------------------
 // Data structure containing the output file is defined and created here. 
 T      = how_long_lasts_trial(parsed_data, nlines);
 if (T <= 0) { error("Zero trial duration !", verbose);  return -1; }
 sprintf(mytext, "Total time: %.2f s @ %.1f Hz", T, srate); msg(mytext, verbose);
 N      = (uint) ceill(T * srate) + nlines - 1;
 (*output) = (double *) calloc(N, sizeof(double));   // Please note: "c"-alloc is indeed required here!
 if ((*output) == NULL) { error("Unable to allocate memory for <output> !", verbose);  return -1; } 

 //--------------------------------------------------------------------------------------

 //--------------------------------------------------------------------------------------
 // Let's start managing the input file..

 current_line = 0;      // Starting from the first entry
 (*index)     = 0;      // Index of the output data structure "output" is set to "0".
 
 while (current_line < nlines) {
 
  if (parsed_data[current_line][CODE] > 0) {
  Ni = (uint) ceill(parsed_data[current_line][DURATION] * srate);
  if (((*index)+Ni) > N) { fprintf(stderr, "%d > %d\n", (*index)+Ni, N); error("Out of range in <output> !", verbose);  return -1; } 
  return_code = simple_waveform(parsed_data[current_line], (*output), index, Ni, srate, dt, verbose); 
  if (return_code == -1) { error("simple_waveform returned -1", verbose); return -1;}
  current_line++;
  }
 else
  {
  i = composite_waveform(parsed_data, current_line, (*output), index, N, srate, dt, verbose); 
  if (i == -1) { error("composite_waveform returned -1", verbose); return -1;}
  current_line += i; 
  }
 
 } // end while()
 //--------------------------------------------------------------------------------------

if (output_on_file) {
fp = fopen(outfilename, "w");
for (i=0; i<(*index); i++)
 fprintf(fp, "%f %f\n", i*dt, (*output)[i]);
}
//--------------------------------------------------------------------------------------
// Allocated memory is released here.
for (i=0; i<MAXROWS; i++)  free(parsed_data[i]);
free(parsed_data);
//free((*output));    // NOT HERE FOR THIS VERSION OF MULTICHANNEL!!
//--------------------------------------------------------------------------------------

return 0;
}
