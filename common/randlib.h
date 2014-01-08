/*=========================================================================
 *
 *   Program:     lcg
 *   Filename:    randlib.h
 *
 *   Copyright (C) 2012,2013,2014 Daniele Linaro
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

#ifndef RANDLIB
#define RANDLIB

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define RANDOMDEVICE	"/dev/urandom"
#define BUFSIZE			8192

typedef unsigned long long ullong;
typedef unsigned int uint;

const double maxint = 4294967296.;
const double maxlong = 18446744073709551616.;

bool shuffle(int start, int stop, int *data);
double gammln(double x);

#define MAXDIM 6
#define MAXBIT 30
#define MIN(a,b) ((a) < (b) ? (a) : (b))
/**
 * When n is negative, internally initializes a set of MAXBIT direction numbers for each
 * of MAXDIM different Sobol sequences. When n is positive (but less than or equal to
 * MAXDIM), returns as the vector x[1..n] the next values from n of these sequences.
 * Note that n must not be changed between initializations.
 * Taken from Numerical Recipes in C - The Art of Scientific Computing, 2nd Edition, Section 7.7.
 */
void sobseq(int *n, float *x);

class UniformRandom { 
private:
	ullong u,v,w; 
public:
	UniformRandom(ullong seed) : v(4101842887655102017LL), w(1) { 
		u = seed ^ v; int64(); 
		v = u; int64(); 
		w = v; int64(); 
	} 
	
	inline ullong int64() { 
		u = u * 2862933555777941757LL + 7046029254386353087LL; 
		v ^= v >> 17;
		v ^= v << 31;
		v ^= v >> 8; 
		w = 4294957665U*(w & 0xffffffff) + (w >> 32); 
		ullong x = u ^ (u << 21); x ^= x >> 35; x ^= x << 4; 
		return (x + v) ^ w;
	}

	inline uint int32() {
		return (uint) int64();
	}

	inline double doub() {
		return 5.42101086242752217E-20 * int64();
	}

        virtual double random() {
                return doub();
        }
};

class NormalRandom : public UniformRandom {
private:
	double mu,sig;
	double storedval;
public:
	NormalRandom(double mmu, double ssig, ullong seed) 
	: UniformRandom(seed), mu(mmu), sig(ssig), storedval(0.) {} 
	
	double random() { 
		double v1,v2,rsq,fac; 
		if (storedval == 0.) {
			do { 
				v1 = 2.0*doub()-1.0;
				v2 = 2.0*doub()-1.0; 
				rsq=v1*v1+v2*v2;
			} while (rsq >= 1.0 || rsq == 0.0); 
			fac = sqrt(-2.0*log(rsq)/rsq);
			storedval = v1*fac; 
			return mu + sig*v2*fac; 
		} else {
			fac = storedval; 
			storedval = 0.; 
			return mu + sig*fac; 
		} 
	} 
};

class PoissonRandom : UniformRandom {
public:
	/** Constructor arguments are lambda and a random sequence seed. */ 
	PoissonRandom(double llambda, ullong seed) :
		UniformRandom(seed), lambda(llambda), lambold(-1.) {
		for (int i=0; i<1024; i++)
			logfact[i] = -1.; 
	}
	
        double random() {
                return (double) irandom();
        }

	/** Return a Poisson deviate using the most recently set value of lambda. */
	int irandom() {
		double u,u2,v,v2,p,t,lfac;
		int k;
		if (lambda < 5.) {	// Will use product of uniforms method.
			if (lambda != lambold)
				lamexp = exp(-lambda);
			k = -1;
			t = 1.;
			do {
				++k;
				t *= doub();
			} while (t > lamexp);
		}
		else {	// Will use ratio of uniforms method.
			if (lambda != lambold) {
				sqlam = sqrt(lambda);
				loglam = log(lambda);
			}
			for (;;) {
				u = 0.64 * doub();
				v = -0.68 + 1.28*doub();
				if (lambda > 13.5) {	// Outer squeeze for fast rejection.
					v2 = v*v;
					if (v >= 0.) {
						if (v2 > 6.5*u*(0.64-u)*(u+0.2)) 
							continue;
					} 
					else {
						if (v2 > 9.6*u*(0.66-u)*(u+0.07)) 
							continue;
					}
				}
				k = (int) (floor(sqlam*(v/u) + lambda + 0.5));
				if (k < 0) 
					continue;
				u2 = u*u;
				if (lambda > 13.5) {	// Inner squeeze for fast acceptance.
					if (v >= 0.) {
						if (v2 < 15.2*u2*(0.61-u)*(0.8-u)) 
							break;
					} else {
						if (v2 < 6.76*u2*(0.62-u)*(1.4-u))
							break;
					}
				}
				if (k < 1024) {
					if (logfact[k] < 0.)
						logfact[k] = gammln(k+1.);
					lfac = logfact[k];
				}
				else
					lfac = gammln(k+1.);
				p = sqlam*exp(-lambda + k*loglam - lfac);	// Only when we must.
				if (u2 < p) 
					break;
			}
		}
		lambold = lambda;
		return k;
	}
	
	/**  Reset lambda and then return a Poisson deviate. */
	int random(double llambda) {
		lambda = llambda;
		return random();
	}
	
private:
	double lambda, sqlam, loglam, lamexp, lambold;
	double logfact[1024];
};

class UniformRandomHW {
public:
	UniformRandomHW(int size = BUFSIZE) {
		bufsize = sizeof(ullong)*size;
		buffer = new unsigned char[bufsize];
		fid = open(RANDOMDEVICE, O_RDONLY);
		FillBuffer();
	}
	
	~UniformRandomHW() {
		close(fid);
		delete buffer;
	}
	
	inline ullong int64() {
		FillUnion();
		return n.aLong;
	}
	
	inline uint int32() {
		FillUnion();
		return n.aInt[0];
	}
	
	inline double doub() {
		return ((double) int64()) / maxlong;
	}
	
private:
	union _number {
		unsigned char s[sizeof(ullong)];
		ullong aLong;
		uint aInt[2];
	};
	
	inline void FillBuffer() {
		read(fid, buffer, bufsize);
		k=0;
	}
	
	inline void FillUnion() {
		if(k > bufsize-(int)sizeof(ullong)) {
			FillBuffer();
		}
		memcpy((void *) &n.s, (const void *) (buffer+k), sizeof(ullong));
		k += sizeof(ullong);
	}
	
	union _number n;
	int bufsize;
	int k;
	int fid;
	unsigned char *buffer;
};

class NormalRandomHW : UniformRandomHW {
private:
	double mu,sig;
	double storedval;
public:
	NormalRandomHW(double mmu, double ssig, ullong bufsize = BUFSIZE) 
	: UniformRandomHW(bufsize), mu(mmu), sig(ssig), storedval(0.) {} 
	
	double random() { 
		double v1,v2,rsq,fac; 
		if (storedval == 0.) {
			do { 
				v1 = 2.0*doub()-1.0;
				v2 = 2.0*doub()-1.0; 
				rsq=v1*v1+v2*v2;
			} while (rsq >= 1.0 || rsq == 0.0); 
			fac = sqrt(-2.0*log(rsq)/rsq);
			storedval = v1*fac; 
			return mu + sig*v2*fac; 
		} else {
			fac = storedval; 
			storedval = 0.; 
			return mu + sig*fac; 
		} 
	} 
};

#endif

