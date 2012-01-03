/***********************************************************************************************
 
 Antwerp, 10/07/2005 - Michele Giugliano, PhD
 Bern, 22/1/2004
 
 file_parsing.c     : library containing procedures used for input file parsing.

 int extract(double *,char *)       :   takes as an input a character string and it extracts
                                        each numerical value, assuming ' ' and '\t' as separa-
                                        tors, and finally returning the number of values found.
                                        
 int readmatrix(char *, double **, INT *, INT *)        : takes advantage of the previous func.
                                                    to process a multi-line text file, repeating
                                                    for every line the 'extract' procedure. Note: a
                                                    line which starts with '%', '/' or '#' is 
                                                    considered a comment.
    
***********************************************************************************************/

#include "file_parsing.h"

//----------------------------------------------------------------
int extract(double *v,char *buf)	{
  int n = 0;
  char *s;

  s=buf;

  // reach the first number (if any) of the current buffer
  while(*s && (*s==' ' || *s=='\t')) s++; // skip blanks
  if(*s==0 || *s=='\n') return 0; // no numbers

  // read the number
  while(*s)
    {
      if (n >= MAXCOLS) { fprintf(stderr, " too many column [check input file format]"); return -1; }
      v[n] = atof(s);
      n++;
      while(*s && !(*s==' ' || *s=='\t')) s++;            // skip non-blanks
      while(*s && (*s==' ' || *s=='\t' || *s=='\n')) s++; // skip blanks
      if(*s==0)  break;
    }
  return n;  // Number of columns of the datafile
} // end extract()
//----------------------------------------------------------------


//----------------------------------------------------------------
int readmatrix(char *file, double **matr, INT *rownum, INT *colnum)	{
  FILE *dev;
  char buf[1000];                                   // tmp buffer
  INT i = 0;
   
  if ((dev = fopen(file, "r")) == NULL)             // The file is open read-only. If it doesn't exist, 
 { fprintf(stderr,"input file not found!"); return -1; }     // a standard error message is printed..

  while(fgets(buf, 1000, dev))                               // Line by line, the file is processed..
   if (buf[0]!='#' && buf[0]!='%' && buf[0]!='/' && strlen(buf)>2) {
    if (i>=MAXROWS) {    fprintf(stderr,"too many lines [increase MAXROWS]"); return -1; }
    *colnum = extract(matr[i],buf);                           // ..by means of the 'extract' routine.
    i++;
    }
                                                            // 'i' counts the number of rows.
  *rownum = i;
    
  fclose(dev); 
  return 0; // normal exit  
} // end readmatrix()
//----------------------------------------------------------------
