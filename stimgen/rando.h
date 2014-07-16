/*=========================================================================
 *
 *   Program:     lcg
 *   Filename:    rando.h
 *
 *   Copyright (C) 2004,2005 Michele Giugliano
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

#ifndef RANDO_H
#define RANDO_H

#ifdef __cplusplus
extern "C" {
#endif

double srand49(unsigned long long seed);
double drand49(void);
double gauss(void);
unsigned long long hw_rand(void);

struct uniform_random {
        unsigned long long seed;
        unsigned long long u, v, w;
};

struct normal_random {
        double mu, sig, storedval;
        struct uniform_random *uniform;
};

void uniform_random_set_seed(struct uniform_random *ran, unsigned long long seed);
struct uniform_random* uniform_random_create(unsigned long long seed);
double uniform_random_value(struct uniform_random *ran);

struct normal_random* normal_random_create(double mu, double sig, unsigned long long seed);
double normal_random_value(struct normal_random *ran);

#ifdef __cplusplus
}
#endif

#endif

