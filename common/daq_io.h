#ifndef DAQ_IO_H
#define DAQ_IO_H

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <comedilib.h>
#include <time.h>
#include <math.h>

#include <vector>

#include "configuration.h"

struct io_thread_arg {
        io_thread_arg(double final_time, double time_step,
                      const std::vector<channel_opts*>* input,
                      const std::vector<channel_opts*>* output)
                : tend(final_time), dt(time_step),
                  input_channels(input), output_channels(output),
                  data(NULL), data_length(0) {}
        ~io_thread_arg() {
                if (data_length)
                        delete data;
        }
        double tend, dt;
        const std::vector<channel_opts*>* input_channels;
        const std::vector<channel_opts*>* output_channels;
        double *data;
        size_t data_length;
};

/**
 * This thread performs I/O using the Comedi library. Its features are:
 * 1) It performs both input and output operations.
 * 2) It assumes that all input and output channels are on the same device.
 * 3) It assumes that all input channels and all output channels are on the
 *    same subdevice (which are different between in and out, obviously).
 * 4) It saves the data to a given memory location.
 */
void* io_thread(void *in);

#endif // DAQ_IO_H

