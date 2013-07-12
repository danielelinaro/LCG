#include "randlib.h"
#include <time.h>

bool shuffle(int start, int stop, int *data)
{
        if (start > stop) {
                return false;
        }

        int i, cnt, r;
        size_t n = stop - start;
        bool *idx = new bool[n+1];
        UniformRandom rnd(time(NULL));

        for (i=0; i<n+1; i++)
                idx[i] = false;

        cnt = 0;
        while (cnt <= n) {
                r = round(start + n*rnd.doub());
                if (!idx[r-start]) {
                        idx[r-start] = true;
                        data[cnt] = r;
                        cnt++;
                }
        }

        delete idx;
        return true;
}

double gammln(double x) {
	int j;
	double xx, tmp, y, ser;
	static const double cof[14] = {
		57.1562356658629235     , -59.5979603554754912     ,
		14.1360979747417471     ,  -0.491913816097620199   ,
		 0.339946499848118887e-4,   0.465236289270485756e-4,
		-0.983744753048795646e-4,   0.158088703224912494e-3,
		-0.210264441724104883e-3,   0.217439618115212643e-3,
		-0.164318106536763890e-3,   0.844182239838527433e-4,
		-0.261908384015814087e-4,   0.368991826595316234e-5
	};
	
	if (x <= 0) 
		throw("bad arg in gammln");
	
	y = xx = x;
	tmp = xx+5.24218750000000000; // Rational 671/128.
	tmp = (xx+0.5) * log(tmp) - tmp;
	ser = 0.999999999999997092;
	for (j=0; j<14; j++)
		ser += cof[j] / ++y;
	return tmp + log(2.5066282746310005*ser/xx);
}

void sobseq(int *n, float *x)
{
        int j,k,l;
        unsigned long i,im,ipp;
        static float fac;
        static unsigned long in,ix[MAXDIM+1],*iu[MAXBIT+1];
        static unsigned long mdeg[MAXDIM+1] = {0,1,2,3,3,4,4};
        static unsigned long ip[MAXDIM+1] = {0,0,1,1,2,1,4};
        static unsigned long iv[MAXDIM*MAXBIT+1] = {0,1,1,1,1,1,1,3,1,3,3,1,1,5,7,7,3,3,5,15,11,5,15,13,9};

        if (*n < 0) { // Initialize, don’t return a vector.
                for (k=1; k<=MAXDIM; k++)
                        ix[k] = 0;
                in = 0;
                if (iv[1] != 1)
                        return;
                fac = 1.0/(1L << MAXBIT);
                for (j=1,k=0; j<=MAXBIT; j++,k+=MAXDIM)
                        iu[j] = &iv[k];
                // To allow both 1D and 2D addressing.
                for (k=1; k<=MAXDIM; k++) {
                        for (j=1; j<=mdeg[k]; j++)
                                iu[j][k] <<= (MAXBIT-j);
                        // Stored values only require normalization.
                        for (j=mdeg[k]+1; j<=MAXBIT; j++) { // Use the recurrence to get other values.
                                ipp = ip[k];
                                i = iu[j-mdeg[k]][k];
                                i ^= (i >> mdeg[k]);
                                for (l=mdeg[k]-1; l>=1; l--) {
                                        if (ipp & 1)
                                                i ^= iu[j-l][k];
                                        ipp >>= 1;
                                }
                                iu[j][k] = i;
                        }
                }
        }
        else {
                im = in++;
                for (j=1; j<=MAXBIT; j++) {
                        if (!(im & 1))
                                break;
                        im >>= 1;
                } 
                if (j > MAXBIT)
                        fprintf(stderr, "MAXBIT (%d) too small in sobseq.\n", MAXBIT);
                im = (j-1)*MAXDIM;
                for (k=1; k<=MIN(*n,MAXDIM); k++) {
                        ix[k] ^= iv[im+k];
                        x[k-1] = ix[k]*fac;
                }
        }
}
