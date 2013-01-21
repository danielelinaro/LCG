/* -------------------------------- rando.c ------------------------------ */
//
// RANDOM NUMBER GENERATORS, FROM NUMERICAL RECIPES - GOOD QUALITY, 32bit-CPUs
//
// Antwerp, 10/07/2009 - Library originally implemented by M. Mattia & S. Fusi;
// based on material available on Numerical Recipes, by Press et al., 1992 - Cambridge U.P.
//
// Used by M. Giugliano for the Mee_Duck *generate_trial* 2001-2009
//

#include <stdio.h>
#include <math.h>
#include "rando.h"

long rand49_idum=-77531;

#define MM 714025
#define IA 1366
#define IC 150889

//----------------------------------------------------------------
float drand49()	{
        static long iy,ir[98];
        static int iff=0;
        int j;

    if (rand49_idum < 0 || iff == 0) {
            iff=1;
            if((rand49_idum=(IC-rand49_idum) % MM)<0)
                             rand49_idum=(-rand49_idum);
            for (j=1;j<=97;j++) {
                    rand49_idum=(IA*(rand49_idum)+IC) % MM;
                    ir[j]=(rand49_idum);
            }
            rand49_idum=(IA*(rand49_idum)+IC) % MM;
            iy=(rand49_idum);
        }
        j=1 + 97.0*iy/MM;
    if (j > 97 || j < 1) fprintf(stderr, "RAN2: This cannot happen.");
        iy=ir[j];
        rand49_idum=(IA*(rand49_idum)+IC) % MM;
        ir[j]=(rand49_idum);
        return (float) iy/MM;
} // end drand49()
//----------------------------------------------------------------


//----------------------------------------------------------------
float srand49(long seed) {
   rand49_idum=(-seed);
   return drand49();
} // end srand49()
//----------------------------------------------------------------


//----------------------------------------------------------------
long mysrand49(long seed) {
  long temp;
  temp = -rand49_idum;
  rand49_idum = (-seed);
  return temp;
} // end mysrand49()
//----------------------------------------------------------------


//----------------------------------------------------------------
float gauss()	{
    static int iset=0;
    static float gset;
    float fac,r,v1,v2;
	
    if  (iset == 0) {
        do {
            v1=2.0*drand49()-1.0;
            v2=2.0*drand49()-1.0;
            r=v1*v1+v2*v2;
        } while (r >= 1.0);
        fac=sqrt(-2.0*log(r)/r);
        gset=v1*fac;
        iset=1;
        return v2*fac;
    } else {
        iset=0;
        return gset;
    }
} // end gauss()
//----------------------------------------------------------------

#undef MM
#undef IA
#undef IC


//----------------------------------------------------------------
// Random bit (i.e. 0, 1) generator, with probability 0.5 and 0.5
#define IB1 1
#define IB2 2
#define IB5 16
#define IB18 131072
#define MASK IB1+IB2+IB5

static unsigned long iseed=31277;

int srand10(long seed)	{
   iseed=seed;
return 0;
} // end srand10()

int drand10()	{
        if (iseed & IB18) {
                iseed=((iseed ^ (MASK)) << 1) | IB1;
                return 1;
        } else {
                iseed <<= 1;
                return 0;
        }
} // end drand10()

#undef MASK
#undef IB18
#undef IB5
#undef IB2
#undef IB1
//----------------------------------------------------------------

