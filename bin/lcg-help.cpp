#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "common.h"

const char lcg_usage_string[] = 
        "LCG is a software for performing electrophysiology experiments developed\n"
        "at the Theoretical Neurobiology and Neuroengineering Laboratory of the\n"
        "University of Antwerp.\n\n"
        "Authors: Daniele Linaro (danielelinaro@gmail.com)\n"
        "         Joao Couto (jpcouto@gmail.com)\n\n"
        "Usage: lcg [--version,-v] [--help,-h] <command> [<args>]\n\n"
        "The most commonly used commands at present available in LCG are:\n";

const char *lcg_commands[] = {
        "   annotate      Add comments to an existing H5 file",
        "   ap            Inject a brief depolarizing pulse of current to elicit a single action potential",
        "   ecode         Perform a series of protocols to characterize the electrophysiological properties of a cell",
        "   experiment    Perform a voltage, current or dynamic clamp experiment described in an XML configuration file",
        "   fclamp        Find the current necessary to make a neuron spike at a given frequency",
        "   fi            Compute an f-I curve using a PID controller",
        "   impedance     Inject subthreshold currents into a neuron to measure its impedance",
        "   kernel        Inject a noisy current for the computation of the kernel used for Active Electrode Compensation",
        "   ou            Inject Ornstein-Uhlenbeck noise into a neuron",
        "   output        Output a constant value to a certain channel of the DAQ board",
        "   pulses        Inject a train of brief pulses of current in a cell to test connectivity",
        "   ramp          Inject a ramp of current into a neuron",
        "   Rin           Inject a series of hyperpolarizing pulses to compute the input resistance of a cell",
        "   spontaneous   Record spontaneous activity from a number of channels on a single subdevice",
        "   steps         Inject steps of voltage or current into a neuron",
        "   stimgen       Write stimulus files",
        "   stimulus      Perform a stimulation defined in a stimulus file",
        "   tau           Inject a brief hyperpolarizing pulse of current to measure the membrane time constant of the neuron",
        "   vcclamp       Perform a voltage or current clamp experiment using a stimulus file",
        "   vi            Inject hyperpolarizing and depolarizing DC steps of current to compute a V-I curve",
        "   zap           Inject a chirp of current to compute the subthreshold response to varying frequencies",
        "   zero          Output zero to all channels of the DAQ board",
        NULL
};

static struct option longopts[] = {
        {"version", no_argument, NULL, 'v'},
        {"help", no_argument, NULL, 'h'},
        {NULL, 0, NULL, 0}
};

void usage()
{
        int i=0;
        printf("%s\n", lcg_usage_string);
        while (lcg_commands[i])
                printf("%s\n", lcg_commands[i++]);
        printf("\nType 'lcg help COMMAND' for more information on a specific command.\n");
}

void parse_args(int argc, char *argv[])
{
        int ch;

        if (argc == 1) {
                usage();
                exit(0);
        }

        while ((ch = getopt_long(argc, argv, "vh", longopts, NULL)) != -1) {
                switch(ch) {
                case 'v':
                       printf("lcg version %s\n", VERSION);
                       break;
                case 'h':
                default:
                       usage();
                       exit(0);
                }
        }
}

int main(int argc, char *argv[])
{
        char cmd[64];
        if (argc > 1 && argv[1][0] != '-') {
                sprintf(cmd, "lcg-%s -h", argv[1]);
                system(cmd);
        }
        else {
                parse_args(argc, argv);
        }
        return 0;
}

