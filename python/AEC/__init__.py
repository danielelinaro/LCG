'''
Active Electrode Compensation (AEC)
Offline electrode kernel estimation and compensation (current-clamp).
R. Brette 2008 (brette@di.ens.fr)

The technique was presented in the following paper:
High-resolution intracellular recordings using a real-time computational model of the electrode
R. Brette, Z. Piwkowska, C. Monier, M. Rudolph-Lilith, J. Fournier, M. Levy, Y. Fregnac, T. Bal, A. Destexhe

--------------------------------
*** Required: scipy package ***
--------------------------------
How to use these functions:
1) Estimation of electrode kernel:
* Experiment: inject white noise current in the cell, record V (potential) and I (current)
* Load the files with V and I
* Extract the full kernel: K=full_kernel(V,I,200)
 (200 = number of points in the kernel; a rule of thumb is membrane time constant/dt)
* Extract the electrode kernel: Ke=electrode_kernel(K,50)
 (50 = number of points in the electrode kernel; typically a few ms / dt)
2) Compensation:
* Experiment: inject the current you want in the cell, record V (potential) and I (current)
* Load the files with V and I
* Apply AEC compensation: V_AEC=AEC_compensate(V,I,Ke)
'''

__all__=['AEC_compensate','full_kernel','electrode_kernel']

from AECoffline import AEC_compensate, full_kernel, electrode_kernel
