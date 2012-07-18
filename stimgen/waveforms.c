/***********************************************************************************************

 Antwerp, 10/07/2005 - Michele Giugliano, PhD
 Bern, 23/11/2003
 Bern, 22/1/2004
 
 waveform.c   : 

***********************************************************************************************/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "waveforms.h"
#include "rando.h"
#include "error_msgs.h"

double how_long_lasts_trial(double **vector, uint nlines) {
/*
  double DT, T = 0.;
  while (nlines > 0) {   
  DT = vector[nlines-1][DURATION];  
  if (DT > 0) T += DT; 
  nlines--;
  }
*/


 double DT, T = 0.;
 uint i, composite = 0;

 for (i=0; i<nlines; i++) {
 
 DT = vector[i][DURATION];
//  if (vector[i][CODE] > 0) {
//   DT = vector[i][DURATION];
//   composite = 0;
//  }
//  else {
//   DT = (composite==0) ? vector[i][DURATION] : 0.;
//   composite = 1;
//  }

 T += DT;

}

return T;
} // end how_long_lasts_trial()



int simple_waveform(double *vector, double *output, uint *index, uint Ni, double srate, double dt, int verbose) {
long oldseed;

//printf("simple_waveform called with code = %d\n", (int) vector[CODE]);

        if (vector[FIXSEED])
                oldseed = mysrand49((long) vector[MYSEED]);
        else
                oldseed = mysrand49(time(NULL));

 switch ((uint) vector[CODE])   // Main decision stage to rule out the requested subwvform type (gauss, DC, etc..).
    {
    case DC_WAVE:                                         // DC Waveform has been selected
      DC(vector[P1], output, index, Ni, vector[EXPON], srate, dt);   // ..then generate it by specifying only the amplitude (coded in P2).
      break;
    case ORNUHL_WAVE:                                                   // GAUSSIAN/ORUHL Waveform has been selected:
      if (vector[P3] <= 0.)                                             // if the correlation time (P3) is 0 ms, use "GAUSS()" for a 
       GAUSS(vector[P1], vector[P2], output, index, Ni, vector[EXPON], srate, dt); // pseudo-delta-correlated process (NOT FEASIBLE ANYWAY).
      else                                                              // Otherwise, the user meant to ask for 
      ORUHL(vector[P1], vector[P2], vector[P3], output, index, Ni, vector[EXPON], srate, dt); // "ORUHL()" routine, so generate the realization.
      break;
    case SINE_WAVE:                                                               // SINUSOIDAL Waveform has been selected
      SINE(vector[P1], vector[P2], vector[P3], output, index, Ni, vector[EXPON], srate, dt); // ..then generate it by specifying the amplitude 
      break;                                                                      //(P2), the frequency (P3) and the phase (P4).
    case POISSON1_WAVE:                                                           // POISSON SHOT 1 Waveform has been selected
      POISSON_SHOT1(vector[P1], vector[P2], vector[P3], output, index, Ni, vector[EXPON], srate, dt); // ..then generate it (either deterministic 
      break;                                                                               // or stochastically) - depends on the sign of (P2).
    case POISSON2_WAVE:                                                                    // POISSON SHOT 2 Waveform has been selected
      POISSON_SHOT2(vector[P1], vector[P2], vector[P3], output, index, Ni, vector[EXPON], srate, dt); // ..then generate it (either deterministic 
      break;                                                                               // or stochastically) - depends on the sign of (P2).
    case BIPOLAR_WAVE:                                                                               // BIPLOAR SHOTS Waveform has been selected
      BIPOLAR_SHOT(vector[P1], vector[P2], vector[P3], output, index, Ni, vector[EXPON], srate, dt);  // ..then generate it (either deterministic 
      break;                                                                               // or stochastically) (..sign of P2).
    case RAMP_WAVE:                                             // RAMP Waveform has been selected
      RAMP(vector[P1], output, index, Ni, vector[EXPON], srate, dt);       // ..then generate it by specifying the 'arrival' amplitude (P1).
      break;
    case SQUARE_WAVE:                                                                // SQUARE Waveform has been selected
      SQUARE(vector[P1], vector[P2], vector[P3], output, index, Ni, vector[EXPON], srate, dt);  // ..then generate it.
      break;                    
    case SAW_WAVE:                                                                // SAW Waveform has been selected
      SAW(vector[P1], vector[P2], vector[P3], output, index, Ni, vector[EXPON], srate, dt);  // ..then generate it.
      break;                    
    case SWEEP_WAVE:                                                                // SWEEP Waveform has been selected
      SWEEP(vector[P1], vector[P2], vector[P3], output, index, Ni, vector[EXPON], srate, dt);  // ..then generate it.
      break;
	case UNIF_NOISE:	
	   UNIFNOISE(vector[P1], vector[P2], output, index, Ni, vector[EXPON], srate, dt); // pseudo-delta-correlated process (NOT FEASIBLE ANYWAY).
	   break;
	case ALPHA_FUN:	
       ALPHA(vector[P1], vector[P2],vector[P3], output, index, Ni, vector[EXPON], srate, dt); // double exponential alpha function process.
	   break;
    default:
      error("Invalid waveform code!", verbose);
      return -1;                   
} // end switch()


if (vector[FIXSEED]) mysrand49(oldseed);

return 0;
} // end simple_waveform()


int composite_waveform(double **parsed_data, uint current_line, double *output, uint *index, uint N, double srate, double dt, int verbose) {
uint i, Ni, line = 0, index_old, myindex, howmany;
double *out_temp, *vector;
int return_code;

Ni       = (uint) (parsed_data[current_line][DURATION] * srate);
out_temp = (double *) calloc(Ni, sizeof(double));
if (out_temp == NULL) { error("Unable to allocate memory for <out_temp> !", verbose);  return -1; }

index_old= (*index);

howmany = (uint) (-parsed_data[current_line][CODE]);

for (line=0; line<howmany; line++)
{
myindex     = 0;
vector      = parsed_data[current_line + line];
vector[CODE]=vector[SUBCODE];
return_code = simple_waveform(vector, out_temp, &myindex, Ni, srate, dt, verbose);
if (return_code == -1) {error("simple_wav returned -1 inside composite_wav.", verbose); return -1;}

(*index) = index_old;
switch ((int) vector[PREC_OP])
 {
 case SUMMATION:
 for (i=0; i<Ni; i++)
 { output[(*index)++] += out_temp[i];      out_temp[i] = 0.;  } 
 break;
 case MULTIPLICATION:
 for (i=0; i<Ni; i++)
 { output[(*index)++] *= out_temp[i];      out_temp[i] = 0.;  } 
 break;
 case SUBTRACTION:
 for (i=0; i<Ni; i++)
 { output[(*index)++] -= out_temp[i];      out_temp[i] = 0.;  } 
 break;
 case DIVISION:
 for (i=0; i<Ni; i++)
 { if (out_temp[i]==0) {error("zero division!", verbose); return -1;} 
 output[(*index)++] /= out_temp[i];      out_temp[i] = 0.;  } 
 break; 
 default:
  for (i=0; i<Ni; i++)
 { output[(*index)++] += out_temp[i];      out_temp[i] = 0.;  } 
 break;

 } // end switch
} // end while

free(out_temp);
return howmany;
} // end composite_waveform()


// Generate DC waveform -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//
void DC(double amplitude, double *output, uint *index, uint Ni, double expon, double srate, double dt)
{
  uint i;
  if (amplitude == 0.)                 // Given that the data structure is initialized to zero when created,
    (*index) += Ni;                    // if the requested amplitude is zero, simply don't do anything! (FASTER!)
  else {
   if ((expon == -1) || (expon == 0) ) amplitude = fabs(amplitude);
   else if (expon != 1) amplitude = pow(amplitude, expon);
  
    for (i=0; i<Ni; i++)               // If that was not the case, then go through the entire data structure
      output[(*index)++] = amplitude;  // and initialize it to the appropriate amplitude value. 
  }     
  return;
} // end DC() -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-



// Generate GAUSSIAN waveform -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//
void GAUSS(double mean, double stdv, double *output, uint *index, uint Ni, double expon, double srate, double dt)
{                                          // This is supposed to be a 'white' gaussian noise!
  uint i;
  if (stdv == 0.) {                        // When the specified stddev is 0., the user meant a DC signal.
    DC(mean, output, index, Ni, expon, srate, dt);    // so I think this is going to be quickly generated by DC().
    return;
  }

 if (expon == -1) 
  for (i=0; i<Ni; i++)                                // Simply go through the entire data structure and initialize
   output[(*index)++] = fabs(mean + stdv * gauss());  // it with a gaussian distribution of "mean" and "stdv".
 else if (expon == 0)
   for (i=0; i<Ni; i++)                                  // Simply go through the entire data structure and initialize
   output[(*index)++] = POSPART(mean + stdv * gauss());  // it with a gaussian distribution of "mean" and "stdv".
 else if (expon != 1)
   for (i=0; i<Ni; i++)                                     // Simply go through the entire data structure and initialize
   output[(*index)++] = pow(mean + stdv * gauss(), expon);  // it with a gaussian distribution of "mean" and "stdv".
 else
   for (i=0; i<Ni; i++)                                // Simply go through the entire data structure and initialize
   output[(*index)++] = mean + stdv * gauss();         // it with a gaussian distribution of "mean" and "stdv".

  return;
} // end GAUSS() -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-



// Generate ORNSTEIN-UHLENBECK waveform -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-==-=-=-=-=-=-
//
void ORUHL(double mean, double stdv, double tau, double *output, uint *index, uint Ni, double expon, double srate, double dt)
{
 double x, tmp1, tmp2, tmp3;
 uint i;
  //
  // Equation implemented: dx = -x*dt/tau + muo*dt + stdvo * sqrt(dt) * gauss()
  // or equivalently:      dx = -x*dt/tau + mean*dt/tau + stdv * sqrt(2*dt/tau) * gauss()
  //
  // Please note: mean represents the -->requested<-- *steady-state* mean of the process..
  //              stdv represents the -->requested<-- *steady-state* stddev of the process..
  //              tau [ms] represents the -->requested<-- correlation time of the process..
  //

  if (stdv == 0.) {                        // When the specified stddev is 0., the user meant a DC signal.
    DC(mean, output, index, Ni, expon, srate, dt);    // so it is going to be quickly generated by DC().
    return;
  }

  x = mean;                                  // The init. condition is set to the desired steady-state mean of
                                             // the output stochastic process (to reduce transients).

  // Please note: although what follows is apparently cryptic, it is just to save ALU loads & CPU time!
  // As already stressed [tau] = ms, therefore  [tau/1000.] = s .  
                        
  tmp1 = dt * 1000./tau;         // I compute (once for all) this quantity, to decrease the ALU overadd..
  tmp2 = mean * tmp1;            // I automatically convert the requested *steady-state* statistical properties,
  tmp3 = stdv * sqrt(2.*tmp1);   // into those variables needed in the main equation.
  
 if (expon == -1) 
  for(i=0; i<Ni; i++) {                       // I go through all the data vector and I initialize it with 
    output[(*index)++] = fabs(x);             // the appropriate value of the state var, coming from the 
    x += tmp2 - (tmp1 * x) + tmp3 * gauss();  // implemented Ornstein-Ulhenbeck stochastic diff. equation.
  }
 else if (expon == 0)
  for(i=0; i<Ni; i++) {                       // I go through all the data vector and I initialize it with 
    output[(*index)++] = POSPART(x);          // the appropriate value of the state var, coming from the 
    x += tmp2 - (tmp1 * x) + tmp3 * gauss();  // implemented Ornstein-Ulhenbeck stochastic diff. equation.
  }
 else if (expon != 1)
  for(i=0; i<Ni; i++) {                       // I go through all the data vector and I initialize it with 
    output[(*index)++] = pow(x, expon);       // the appropriate value of the state var, coming from the 
    x += tmp2 - (tmp1 * x) + tmp3 * gauss();  // implemented Ornstein-Ulhenbeck stochastic diff. equation.
  }
 else
  for(i=0; i<Ni; i++) {                       // I go through all the data vector and I initialize it with 
    output[(*index)++] = x;                   // the appropriate value of the state var, coming from the 
    x += tmp2 - (tmp1 * x) + tmp3 * gauss();  // implemented Ornstein-Ulhenbeck stochastic diff. equation.
  }

  return;
} //end ORUHL() -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-



// Generate SINUSOIDAL waveform -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-==-=-=-=-=-=-=-
//
void SINE(double amplitude, double frequency, double phase, double *output, uint *index, uint Ni, double expon, double srate, double dt)
{
 uint i;
 double tmp;

 if (frequency == 0.) {                   // When the specified frequency is 0., the user meant a DC signal.
  DC(amplitude, output, index, Ni, expon, srate, dt);// so it is going to be quickly generated by DC().
  return;
 }

  tmp = TWOPI * frequency;          // This is just to compute once for all such terms (: 2*PI*freq )
                                    // Please note that *phase* is expressed in rad.


 if (expon == -1) 
  for (i=0; i<Ni; i++)              // I go through the entire data vector and I initialize it appropriately.
    output[(*index)++] = fabs(amplitude * sin(  tmp * i * dt + phase ));
 else if (expon == 0)
  for (i=0; i<Ni; i++)              // I go through the entire data vector and I initialize it appropriately.
    output[(*index)++] = POSPART(amplitude * sin(  tmp * i * dt + phase ));
 else if (expon != 1)
  for (i=0; i<Ni; i++)              // I go through the entire data vector and I initialize it appropriately.
    output[(*index)++] = pow(amplitude * sin(  tmp * i * dt + phase ), expon);
 else
  for (i=0; i<Ni; i++)              // I go through the entire data vector and I initialize it appropriately.
    output[(*index)++] = amplitude * sin(  tmp * i * dt + phase );

  return;
} // end SINE()  -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-



// Generate POISSON SHOT 1 waveform -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-==-=-=-=-=-=-
//
void POISSON_SHOT1(double amplitude, double frequency, double width, double *output, uint *index, uint Ni, double expon, double srate, double dt)
{
 double tmp;
 uint i,o,m,j,k;
 
  if (frequency == 0.) {          // Given that the data structure is initialized to zero when created,
    DC(0., output, index, Ni, expon, srate, dt);// if the requested frequency is zero, simply don't do anything! (FASTER!)
    return;
  }

 if ((expon == -1) || (expon == 0))
   amplitude = fabs(amplitude);
 else if (expon != 1)
   amplitude = pow(amplitude, expon);

  m   = (uint) (width * srate / 1000.); // How many points constitute the pulse width ? ([width] = ms)
  tmp = srate / frequency;             // Just to save CPU time for arithmetic operations..
  
  if (frequency > 0.) {

  j = 0;
  while (j<Ni) {  
    k = (uint) ( -log(drand49()) * tmp );
    o = j + k;                       // This variable contains the index at the beginning of the event.
    if ((o+m) < Ni) {                // However such an index + the duration of the pulse might exceed n:
      for(i=0; i<k; i++)             // when this does not happen, simply go through the vector and place
    output[(*index)++] = 0.;         // zeros from the old j to the new one (j + k) and generate the event
      for(i=0; i<m; i++)             // by filling m points (duration of the square pulse event) with
    output[(*index)++] = amplitude;  // the appropriate amplitude value
      j = o + m;                     // Finally update the current index, to the end of the event.
    }
    else {                           // When the drawn ISI is larger or equal to n,
      //for(i=0; i<(n-j); i++)             // then fill the remaining part of vector with zeros.
      //DATA[qq++] = 0.;                   // Although it might at first seems I should not do anything, as the vector
      (*index) = (*index) + (Ni-j);
      j = Ni;                              // is already filled with 0s, we have anyway to update qq!!!
    }                                      // Simply set j to n, so we escape from the while loop and return.
  }
  } // end if

  else {

        /* Modified by Daniele Linaro on Jun 25, 2012 */
        /* START */
        k = (uint) -tmp;       // period, in number of samples
        i = -1;                 // number of periods
        for (j=0; j<Ni; j++, (*index)++) {
                if (j%k == 0)
                        i++;
                if ((j-i*k)%m == (j-i*k))
                        output[*index] = amplitude;
        }
        /* END */

  } // end if

  return;
} // end POISSON_SHOT1() -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-



// Generate BIPOLAR SHOT waveform -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-==-=-=-=-=-=-
//
void BIPOLAR_SHOT(double amplitude, double frequency, double width, double *output, uint *index, uint Ni, double expon, double srate, double dt)
{
 double tmp;
 uint i,o,m,j,k;

  if (frequency == 0.) {          // Given that the data structure is initialized to zero when created,
    DC(0., output, index, Ni, expon, srate, dt);// if the requested frequency is zero, simply don't do anything! (FASTER!)
    return;
  }

 if ((expon == -1) || (expon == 0))
   amplitude = fabs(amplitude);
 else if (expon != 1)
   amplitude = pow(amplitude, expon);


  m   = (uint) (width * srate / 1000.); // How many points constitute the pulse width ? ([width] = ms)
  tmp = srate / frequency;             // Just to save CPU time for arithmetic operations..

  if (frequency > 0.) {

  j = 0;
  while (j<Ni) {  
    k = (uint) ( -log(drand49()) * tmp );
    o = j + k;                             // This variable contains the index at the beginning of the event.
    if ((o+m+m) < Ni) {                    // However such an index + the duration of the pulse might exceed n:
      for(i=0; i<k; i++)                   // when this does not happen, simply go through the vector and place
    output[(*index)++] = 0.;               // zeros from the old j to the new one (j + k) and generate the event
      for(i=0; i<m/2; i++)                 // by filling m/2 points (half of the duration of the bipolar square pulse)
    output[(*index)++] = amplitude;        // with the appropriate amplitude value and 
      for(i=m/2; i<m; i++)                 // by filling the remaining points with the appropriate negative amplitude
    output[(*index)++] = -amplitude;       // value.
      j = o + m;                           // Finally update the current index, to the end of the event.
    }
    else {                                 // When the drawn ISI is larger or equal to n,
      //for(i=0; i<(Ni-j); i++)               // then fill the remaining part of vector with zeros.
      //DATA[qq++] = 0.;                     // Although it might at first seems I should not do anything, as the vector
      (*index) = (*index) + (Ni-j);
      j = (*index);                          // is already filled with 0s, we have anyway to update qq!!!
    }                                        // Simply set j to n, so we escape from the while loop and return.
  }
  } // end if
 
 else {

  j = 0;
  while (j<Ni) {  
    k = (uint) ( -tmp );
    o = j + k;                             // This variable contains the index at the beginning of the event.
    if ((o+m+m) < Ni) {                    // However such an index + the duration of the pulse might exceed n:
      for(i=0; i<k; i++)                   // when this does not happen, simply go through the vector and place
    output[(*index)++] = 0.;               // zeros from the old j to the new one (j + k) and generate the event
      for(i=0; i<m/2; i++)                 // by filling m/2 points (half of the duration of the bipolar square pulse)
    output[(*index)++] = amplitude;        // with the appropriate amplitude value and 
      for(i=m/2; i<m; i++)                 // by filling the remaining points with the appropriate negative amplitude
    output[(*index)++] = -amplitude;       // value.
      j = o + m;                           // Finally update the current index, to the end of the event.
    }
    else {                                 // When the drawn ISI is larger or equal to n,
      //for(i=0; i<(n-j); i++)               // then fill the remaining part of vector with zeros.
      //DATA[qq++] = 0.;                     // Although it might at first seems I should not do anything, as the vector
      (*index) = (*index) + (Ni-j);
      j = Ni;                                // is already filled with 0s, we have anyway to update qq!!!
    }                                        // Simply set j to n, so we escape from the while loop and return.
 
  }

  } // end if

  return;
} // end BIPOLAR_SHOT() -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-



// Generate POISSON SHOT 2 waveform -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-==-=-=-=-=-=-
//
void POISSON_SHOT2(double amplitude, double frequency, double tau, double *output, uint *index, uint Ni, double expon, double srate, double dt)
{
  double *SHOT;           // Temporary array containing the events (generated in advance).
  double x, tmp1, tmp2;
  uint i,o,m,j,k,kk,nn;

  if (frequency == 0.) {          // Given that the data structure is initialized to zero when created,
    DC(0., output, index, Ni, expon, srate, dt);// if the requested frequency is zero, simply don't do anything! (FASTER!)
    return;
  }

  if (tau < (dt*1000.)) {    // If the user erroneously introduced an invalid tau, I set automatically a min
    tau = 10.*(dt*1000.);    // tau of 10 times the sampling time interval.
 // printf("POISSON SHOT 2: an invalid tau has been introduced (tau < dt). Now, tau := 10 dt.");
  }
      
  // First I allocate memory for the event-vector (by using the mxCalloc instead of the calloc, as recommended).
  SHOT = (double *) malloc(5*(uint)(fabs(frequency)*Ni*dt) * sizeof(double));
  
  x = 0.;                                                        // I initialize to zero, the state variable.
  tmp1 = srate / frequency;                                      // Just to save CPU time for arithmetic operations..
  tmp2 = 1. - (dt * 1000. / tau);

  j  = 0;         nn = 0;

  if (frequency > 0.) {

  while (j<Ni) {  
    k = (uint) ( -log(drand49()) * tmp1 );
    k = (k==0) ? 1 : k;                        // This is needed otherwise strange things occur!
    o = j + k;
    if (o < Ni) {
      SHOT[nn++] = o;
      j = o;
    }
    else 
      j = Ni;
  }
 
  } // end if
  else {
 
 while (j<Ni) {  
    k = (uint) ( -tmp1 );
    o = j + k;
    if ( o < Ni) {
      SHOT[nn++] = o;
      j = o;
    }
    else 
      j = Ni;
  }

  } // end if

  //-----------------------------------------------------------------------------------
  if (nn == 0) {              // No event has been generated (the frequency is too low compared to duration)
    // for(i=0; i<n; i++)       // we should therefore return a zero-containing data array.
    // DATA[qq++] = 0.;         // But again we don't need to explicitly fill with 0 the whole output vector,
    (*index) = (*index) + Ni;
    free(SHOT);
    return;                   // as it is already initialized to zero, at the creation time
  }
  //-----------------------------------------------------------------------------------
  
  j  = 0;
  kk = 0;
  while (j<Ni) {  
    if (kk == nn) {             // There is no (more) event in the SHOT data structure.
      for(i=0; i<(Ni-j); i++) { // Therefore we go through the entire data vector and
//    output[(*index)++]  = x;    // we initialize it with the appropriate values returned
      if ((expon == -1) || (expon == 0))  output[(*index)++]  = fabs(x);
       else if (expon != 1) output[(*index)++]  = pow(x, expon);
        else output[(*index)++]  = x;
    x = tmp2 * x;               // by a simple exponential decay.. (first order ode).
      }
      j = Ni;
    }
    else {                    // Still there are events in the SHOT data structure.
//      output[(*index)++] = x; //
      if ((expon == -1) || (expon == 0))  output[(*index)++]  = fabs(x);
       else if (expon != 1) output[(*index)++]  = pow(x, expon);
        else output[(*index)++]  = x;

      if (SHOT[kk] == j) {    // If the event is occurring now, then let's process it.
    x = amplitude;
    kk++;
      }
      else
      x = tmp2 * x;            // Exponential decay..
      j++;
    }
  }
  free(SHOT);
  return;
} // end POISSON_SHOT2() -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-




// Generate RAMP waveform -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-==-=-=-=-=-=-=-=-=-=-=-=-
//
void RAMP(double y_final, double *output, uint *index, uint Ni, double expon, double srate, double dt)
{
 uint i;
 double tmp1, tmp2;
 
  tmp1 = ((*index) == 0) ? 0. : output[(*index)-1]; // The y_init is taken from the last value of DATA

  if (y_final == tmp1) {                      // If the slope is zero, probably the user meant a DC signal.
    DC(tmp1, output, index, Ni, expon, srate, dt);
    return;
  }

  tmp2 = (y_final - tmp1) / Ni;              // I compute in advance this quantity (i.e. the slope)..


 if (expon == -1) 
  for(i=0; i<Ni; i++)
    output[(*index)++] = fabs(tmp1 + tmp2 * i);
 else if (expon == 0)
  for(i=0; i<Ni; i++)
    output[(*index)++] = POSPART(tmp1 + tmp2 * i);
 else if (expon != 1)
  for(i=0; i<Ni; i++)
    output[(*index)++] = pow(tmp1 + tmp2 * i, expon);
 else
  for(i=0; i<Ni; i++)
    output[(*index)++] = tmp1 + tmp2 * i;

  return;
} // end RAMP()  -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-



// Generate SQUARE waveform -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-==-=-=-=-=-=-=-
// OK TESTED
void SQUARE(double amplitude, double frequency, double dutycycle, double *output, uint *index, uint Ni, double expon, double srate, double dt)
{
 double tmp1, tmp2, tmp3;
 uint i;
 
  if (frequency == 0.) {               // Given that the data structure is initialized to zero when created,
    DC(amplitude/2., output, index, Ni, expon, srate, dt);// if the requested frequency is zero, simply don't do anything! (FASTER!)
    return;
  }

  tmp1 = (1./frequency)/dt;              // This is just to compute once for all such terms (: 1./freq )
  tmp2 = (dutycycle * 0.01 * tmp1);      // This is just to compute once for all such terms (: dutycycle * 1./freq )
  tmp3 = 2.*amplitude;                      // Here is the amplitude of the sinusoidal waveform.


 if (expon == -1) 
  for (i=0; i<Ni; i++)               // I go through the entire data vector and I initialize it appropriately.
    output[(*index)++] = fabs(tmp3 * (fmod(i, tmp1)<tmp2) - amplitude); 
 else if (expon == 0)
  for (i=0; i<Ni; i++)               // I go through the entire data vector and I initialize it appropriately.
    output[(*index)++] = POSPART(tmp3 * (fmod(i, tmp1)<tmp2) - amplitude); 
 else if (expon != 1)
  for (i=0; i<Ni; i++)               // I go through the entire data vector and I initialize it appropriately.
    output[(*index)++] = pow(tmp3 * (fmod(i, tmp1)<tmp2) - amplitude, expon); 
 else
  for (i=0; i<Ni; i++)               // I go through the entire data vector and I initialize it appropriately.
    output[(*index)++] = tmp3 * (fmod(i, tmp1)<tmp2)  - amplitude; 

  return;
} // end SQUARE()  -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-



// Generate SAW waveform -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-==-=-=-=-=-=-=-
// OK TESTED 
void SAW(double amplitude, double frequency, double dutycycle, double *output, uint *index, uint Ni, double expon, double srate, double dt)
{ 
 uint i;
 double tmp1, tmp2, tmp3, tmp4; 
 
  if (frequency == 0.) {               // Given that the data structure is initialized to zero when created,
    DC(amplitude/2., output, index, Ni, expon, srate, dt);  // if the requested frequency is zero, simply don't do anything! (FASTER!)
    return;
  }

  tmp1 = (1./frequency)/dt;              // This is just to compute once for all such terms (: 1./freq )
  tmp2 = (dutycycle * 0.01 * tmp1);      // This is just to compute once for all such terms (: dutycycle * 1./freq )
  tmp3 = amplitude / tmp2;               // Here is the amplitude of the sinusoidal waveform.
  tmp4 = amplitude / ((1. - dutycycle * 0.01) * tmp1);// This is just to compute once for all such terms (: dutycycle * 1./freq )
  
 if (expon == -1) 
  for (i=0; i<Ni; i++)             // I go through the entire data vector and I initialize it appropriately.
    output[(*index)++] = fabs(2. * ((fmod(i, tmp1) <= (tmp2)) ? tmp3 * fmod(i, tmp1) : tmp4 * fmod(Ni-i, tmp1))  - amplitude); 
 else if (expon == 0)
  for (i=0; i<Ni; i++)             // I go through the entire data vector and I initialize it appropriately.
    output[(*index)++] = POSPART(2.*((fmod(i, tmp1) <= (tmp2)) ? tmp3 * fmod(i, tmp1) : tmp4 * fmod(Ni-i, tmp1)) - amplitude); 
 else if (expon != 1)
  for (i=0; i<Ni; i++)             // I go through the entire data vector and I initialize it appropriately.
    output[(*index)++] = pow(2.*((fmod(i, tmp1) <= (tmp2)) ? tmp3 * fmod(i, tmp1) : tmp4 * fmod(Ni-i, tmp1)) - amplitude, expon); 
 else
  for (i=0; i<Ni; i++)             // I go through the entire data vector and I initialize it appropriately.
    output[(*index)++] = 2.*((fmod(i, tmp1) <= (tmp2)) ? tmp3 * fmod(i, tmp1) : tmp4 * fmod(Ni-i, tmp1))  - amplitude; 

  return;
} // end SAW()  -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-



// Generate SWEEP waveform -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-==-=-=-=-=-=-=-
// OK TESTED
void SWEEP(double amplitude, double frequency_start, double frequency_stop, double *output, uint *index, uint Ni, double expon, double srate, double dt)
{
 uint i;
 double tmp1, tmp2, tmp3;

  tmp1 = TWOPI * frequency_start;   // This is just to compute once for all such terms (: 2*PI*freq )
  tmp2 = TWOPI * (frequency_stop - frequency_start);
  tmp3 = amplitude;                 // Here is the amplitude of the sinusoidal waveform.

 if (expon == -1) 
  for (i=0; i<Ni; i++)               // I go through the entire data vector and I initialize it appropriately.
    output[(*index)++] = fabs(tmp3 * sin(  (tmp1 + (double)i/(Ni) * tmp2 * 0.5) * i * dt));    
 else if (expon == 0)
  for (i=0; i<Ni; i++)               // I go through the entire data vector and I initialize it appropriately.
    output[(*index)++] = POSPART(tmp3 * sin(  (tmp1 + (double)i/(Ni) * tmp2 * 0.5) * i * dt));    
 else if (expon != 1)
  for (i=0; i<Ni; i++)               // I go through the entire data vector and I initialize it appropriately.
    output[(*index)++] = pow(tmp3 * sin(  (tmp1 + (double)i/(Ni) * tmp2 * 0.5) * i * dt), expon);    
 else
  for (i=0; i<Ni; i++)               // I go through the entire data vector and I initialize it appropriately.
    output[(*index)++] = tmp3 * sin(  (tmp1 + (double)i/(Ni) * tmp2 * 0.5) * i * dt);    
        
  return;
} // end SWEEP()  -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-


// Generate UNIFORM NOISE waveform -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//
void UNIFNOISE(double mean, double stdv, double *output, uint *index, uint Ni, double expon, double srate, double dt)
{                                          // This is supposed to be a 'white' gaussian noise!
	uint i;
	if (stdv == 0.) {                        // When the specified stddev is 0., the user meant a DC signal.
		DC(mean, output, index, Ni, expon, srate, dt);    // so I think this is going to be quickly generated by DC().
		return;
	}
	
	if (expon == -1) 
		for (i=0; i<Ni; i++)                                // Simply go through the entire data structure and initialize
			output[(*index)++] = fabs(mean + stdv * 3.464101615137754 *(drand49() - 0.5));  // Note: if 'r' is uniform, then its mean is 0.5 and its stdev is 1/sqrt(12) !!
	else if (expon == 0)
		for (i=0; i<Ni; i++)                                  // Simply go through the entire data structure and initialize
			output[(*index)++] = POSPART(mean + stdv * 3.464101615137754 *(drand49() - 0.5));  // Note: if 'r' is uniform, then its mean is 0.5 and its stdev is 1/sqrt(12) !!
	else if (expon != 1)
		for (i=0; i<Ni; i++)                                     // Simply go through the entire data structure and initialize
			output[(*index)++] = pow(mean + stdv * 3.464101615137754 *(drand49() - 0.5), expon);  // Note: if 'r' is uniform, then its mean is 0.5 and its stdev is 1/sqrt(12) !!
	else
		for (i=0; i<Ni; i++)                                // Simply go through the entire data structure and initialize
			output[(*index)++] = mean + stdv * 3.464101615137754 *(drand49() - 0.5);   // Note: if 'r' is uniform, then its mean is 0.5 and its stdev is 1/sqrt(12) !!
	return;
} // end UNIFNOISE() -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

// Generate ALPHA waveform -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
void ALPHA(double amplitude, double Trise, double Tdecay, double *output, uint *index, uint Ni, double expon, double srate, double dt)
{
    Trise = Trise/1000.0;               //for consistency taus are given in ms.
    Tdecay = Tdecay/1000.0;
    // first compute the normalization factor (max of the function).
    double tt = (Trise*Tdecay/(Tdecay-Trise)) * log(Tdecay/Trise); // time at which the funtion has a maximum.
    double Nfactor = (exp(-tt/Trise)-exp(-tt/Tdecay))/(Trise-Tdecay); // max value.
    //printf("Amplitude is %lf\n",amplitude);
    uint i;
    if (amplitude == 0.) {                        // When the specified stddev is 0., the user meant a DC signal.
        DC(0.0, output, index, Ni, expon, srate, dt);    // so I think this is going to be quickly generated by DC().
    return;
    }
    // from Phase response curves in Neuroscience, Page 102, accepts time in seconds(for this formula).
    if (expon == -1) 
        for (i=0; i<Ni; i++)                                // Simply go through the entire data structure and initialize
            output[(*index)++] = fabs(amplitude*(1.0/(Nfactor*(Tdecay-Trise)))*(exp(-((i/srate)/Tdecay))-exp(-((i/srate)/Trise))));       //the abs.
    else if (expon == 0)
        for (i=0; i<Ni; i++)                                  // Simply go through the entire data structure and initialize
            output[(*index)++] = POSPART(amplitude*(1.0/(Nfactor*(Tdecay-Trise)))*(exp(-((i/srate)/Tdecay))-exp(-((i/srate)/Trise))));    //and same but only the pospart.
    else if (expon != 1)
        for (i=0; i<Ni; i++)                                     // Simply go through the entire data structure and initialize
            output[(*index)++] = pow(amplitude*(1.0/(Nfactor*(Tdecay-Trise)))*(exp(-((i/srate)/Tdecay))-exp(-((i/srate)/Trise))), expon);  //to the power of.
    else
        for (i=0; i<Ni; i++)                                // Simply go through the entire data structure and initialize
            output[(*index)++] = amplitude*(1.0/(Nfactor*(Tdecay-Trise)))*(exp(-((i/srate)/Tdecay))-exp(-((i/srate)/Trise)));         //double exp alpha function.
    return;
} // end ALPHA() -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
