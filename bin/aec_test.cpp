#include <cstdio>
#include <cstdlib>
#include "aec.h"

#define KERNEL_FILE   "aec_test.k"
#define VR_FILE       "aec_test.vr"
#define VC_FILE       "aec_test.vc"
#define I_FILE        "aec_test.i"
#define KERNEL_LENGTH 100
#define DATA_LENGTH   10000

#define FILE_CONSTRUCTOR

using namespace dynclamp;

int main()
{
        int i;
        double I, Vr, Vc;
        FILE *I_fid, *Vr_fid, *Vc_fid;

#ifndef FILE_CONSTRUCTOR
        double kernel[KERNEL_LENGTH];
        FILE *k_fid;
#endif

        Vr_fid = fopen(VR_FILE, "r");
        if (Vr_fid == NULL) {
                fprintf(stderr, "Unable to open %s.\n", VR_FILE);
                exit(1);
        }

        I_fid = fopen(I_FILE, "r");
        if (I_fid == NULL) {
                fprintf(stderr, "Unable to open %s.\n", I_FILE);
                fclose(Vr_fid);
                exit(1);
        }

        Vc_fid = fopen(VC_FILE, "w");
        if (Vc_fid == NULL) {
                fprintf(stderr, "Unable to open %s.\n", VC_FILE);
                fclose(Vr_fid);
                fclose(I_fid);
                exit(1);
        }

#ifdef FILE_CONSTRUCTOR
        AEC aec(KERNEL_FILE);
#else
        k_fid = fopen(KERNEL_FILE, "r");
        if (k_fid == NULL) {
                fprintf(stderr, "Unable to open %s.\n", KERNEL_FILE);
                fclose(Vr_fid);
                fclose(I_fid);
                fclose(Vc_fid);
                exit(1);
        }
        for (i=0; i<KERNEL_LENGTH; i++)
                fscanf(k_fid, "%le\n", &kernel[i]);
        fclose(k_fid);
        AEC aec(kernel, KERNEL_LENGTH);
#endif

        for (i=0; i<DATA_LENGTH; i++) {
                fscanf(I_fid, "%le\n", &I);
                fscanf(Vr_fid, "%le\n", &Vr);
                aec.pushBack(I);
                Vc = aec.compensate(Vr);
                fprintf(Vc_fid, "%e\n", Vc);
        }

        fclose(I_fid);
        fclose(Vr_fid);
        fclose(Vc_fid);

        return 0;
}

