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
        "The most commonly used lcg commands are:\n"
        "   experiment    performs a (possibly) hybrid or closed loop experiment\n"
        "                 described in an XML configuration file\n"
        "   vcclamp       performs a voltage or current clamp experiment using\n"
        "                 a stim file\n"
        "   annotate      adds comments into an existing H5 file\n\n"
        "Type 'lcg help <command>' for more information on a specific command.\n";

static struct option longopts[] = {
        {"version", no_argument, NULL, 'v'},
        {"help", no_argument, NULL, 'h'},
        {NULL, 0, NULL, 0}
};

void usage()
{
        printf(lcg_usage_string);
}

void parseArgs(int argc, char *argv[])
{
        int ch;
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
                parseArgs(argc, argv);
        }
        return 0;
}

