#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include "common.h"
#include "types.h"
#include <vector>
#include <stdio.h>

typedef enum {INPUT, OUTPUT} channel_type;

struct channel_opts {
        channel_opts(channel_type in_or_out, const char *dev, uint subdev, uint rng, uint ref,
                uint chan, double factor, const char *unts, const char *filename = NULL)
                : type(in_or_out), subdevice(subdev), range(rng), reference(ref),
                  conversionFactor(factor), channel(chan) {
                strncpy(device, dev, FILENAME_MAXLEN);
                strncpy(units, unts, 10);
                stimfile[0] = 0;
                if (filename)
                        strncpy(stimfile, filename, FILENAME_MAXLEN);
        }
        channel_type type;
        char device[FILENAME_MAXLEN];
        uint subdevice, range, reference;
        double conversionFactor;
        uint channel;
        char units[10];
        char stimfile[FILENAME_MAXLEN];
};

int parse_configuration_file(const char *filename, std::vector<channel_opts*>& channels);

void print_channel_opts(channel_opts* opts)
{
        switch(opts->type) {
        case INPUT:
                printf("Device type: INPUT\n");
                break;
        case OUTPUT:
                printf("Device type: OUTPUT\n");
                break;
        default:
                printf("Unknown device type.\n");
                return;
        }
        printf("Device: %s\n", opts->device);
        printf("Subdevice: %d\n", opts->subdevice);
        printf("Range: %d\n", opts->range);
        printf("Reference: %d\n", opts->reference);
        printf("Conversion factor: %g\n", opts->conversionFactor);
        printf("Channel: %d\n", opts->channel);
        printf("Units: %s\n", opts->units);
        if (strlen(opts->stimfile))
                printf("Stimfile: %s\n", opts->stimfile);
        printf("\n");
}

#endif

