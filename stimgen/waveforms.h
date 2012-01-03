/***********************************************************************************************

 Antwerp, 10/07/2005 - Michele Giugliano, PhD
 Bern, 23/11/2003
 Bern, 22/1/2004
 
 waveform.h   : 

***********************************************************************************************/

#ifndef WAVEFORMS_H
#define WAVEFORMS_H

#include "stimgen_common.h"

double how_long_lasts_trial(double **, INT);
int    simple_waveform(double *, double *, INT *, INT, double, double, int);
int    composite_waveform(double **, INT, double *, INT *, INT, double, double, int);

void DC(double, double *, INT *, INT, double, double, double);
void GAUSS(double, double, double *, INT *, INT, double, double, double); 
void ORUHL(double, double, double, double *, INT *, INT, double, double, double);
void SINE(double, double, double, double *, INT *, INT, double, double, double);
void POISSON_SHOT1(double, double, double, double *, INT *, INT, double, double, double);
void POISSON_SHOT2(double, double, double, double *, INT *, INT, double, double, double);
void BIPOLAR_SHOT(double, double, double, double *, INT *, INT, double, double, double);
void RAMP(double, double *, INT *, INT, double, double, double);      
void SQUARE(double, double, double, double *, INT *, INT, double, double, double);
void SAW(double, double, double, double *, INT *, INT, double, double, double); 
void SWEEP(double, double, double, double *, INT *, INT, double, double, double);
void UNIFNOISE(double, double, double *, INT *, INT, double, double, double); 

#endif

