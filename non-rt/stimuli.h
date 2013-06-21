#ifndef STIMULI_H
#define STIMULI_H

#define MAX_NO_STIMULI 64

#include "types.h"

int allocate_stimuli(uint nstim);
void free_stimuli(uint nstim);

#endif

