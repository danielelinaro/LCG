#ifndef RANDO_H
#define RANDO_H

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

#endif

