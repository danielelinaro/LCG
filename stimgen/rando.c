/* -------------------------------- rando.c ------------------------------ */
//
// Random number generators for 64bit CPUs, from Numerical Recipes 3.
// 
// Antwerp, 16/07/2014 - Daniele Linaro
//

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <fcntl.h>
#include <unistd.h>
#include "rando.h"

static unsigned long long int64(struct uniform_random *ran) { 
        unsigned long long x;
        ran->u = ran->u * 2862933555777941757LL + 7046029254386353087LL; 
	ran->v ^= ran->v >> 17;
	ran->v ^= ran->v << 31;
	ran->v ^= ran->v >> 8; 
	ran->w = 4294957665U*(ran->w & 0xffffffff) + (ran->w >> 32); 
	x = ran->u ^ (ran->u << 21); x ^= x >> 35; x ^= x << 4; 
	return (x + ran->v) ^ ran->w;
}

static unsigned int int32(struct uniform_random *ran) {
        return (unsigned int) int64(ran);
}

unsigned long long hw_rand(void) {
        unsigned long long seed;
        int nbytes = 0, fd = open("/dev/urandom",O_RDONLY);
        if (fd == -1)
                return (unsigned long long) time(NULL);
        read(fd, (void *) &seed, sizeof(unsigned long long));
        close(fd);
        return seed;
}

void uniform_random_set_seed(struct uniform_random *ran, unsigned long long seed) {
        ran->seed = seed;
        ran->v = 4101842887655102017LL;
        ran->w = 1;
        ran->u = ran->seed ^ ran->v; int64(ran); 
        ran->v = ran->u; int64(ran); 
        ran->w = ran->v; int64(ran); 
}

struct uniform_random* uniform_random_create(unsigned long long seed) {
        struct uniform_random *ran;
        ran = (struct uniform_random *) malloc(sizeof(struct uniform_random));
        uniform_random_set_seed(ran, seed);
        return ran;
}

double uniform_random_value(struct uniform_random *ran) {
	return 5.42101086242752217E-20 * int64(ran);
}

struct normal_random* normal_random_create(double mu, double sig, unsigned long long seed) {
        struct normal_random *ran;
        ran = (struct normal_random *) malloc(sizeof(struct normal_random));
        ran->mu = mu;
        ran->sig = sig;
        ran->storedval = 0.0;
        ran->uniform = uniform_random_create(seed);
        return ran;
}

double normal_random_value(struct normal_random *ran) {
	double v1,v2,rsq,fac; 
	if (ran->storedval == 0.) {
		do { 
			v1 = 2.0*uniform_random_value(ran->uniform)-1.0;
			v2 = 2.0*uniform_random_value(ran->uniform)-1.0; 
			rsq=v1*v1+v2*v2;
		} while (rsq >= 1.0 || rsq == 0.0); 
		fac = sqrt(-2.0*log(rsq)/rsq);
		ran->storedval = v1*fac; 
		return ran->mu + ran->sig*v2*fac; 
	} else {
		fac = ran->storedval; 
		ran->storedval = 0.; 
		return ran->mu + ran->sig*fac; 
	} 
}

struct uniform_random global_uniform_random;
struct normal_random global_normal_random = {
        .mu = 0.,
        .sig = 1.,
        .storedval = 0.,
        .uniform = &global_uniform_random
};

double srand49(unsigned long long seed) {
        uniform_random_set_seed(&global_uniform_random, seed);
        global_normal_random.storedval = 0.;
}

double drand49(void) {
        return uniform_random_value(&global_uniform_random);
}

double gauss(void) {
        return normal_random_value(&global_normal_random);
}

