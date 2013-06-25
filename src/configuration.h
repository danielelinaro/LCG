#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include "common.h"
#include "types.h"

struct io_options {
        io_options() : channels(NULL) {}
        ~io_options() {
                if (channels)
                        free(channels);
        }
        char device[FILENAME_MAXLEN];
        uint subdevice, range, reference;
        double conversionFactor;
        uint *channels, nchan;
        char units[10];
};

int parse_configuration_file(const char *filename, io_options *input, io_options *output);

#endif

