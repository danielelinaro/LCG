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

#include "stimulus.h"

class Channel {
public:
        Channel(const char *device, uint channel, double samplingRate, const char *units);
        virtual ~Channel();
        const char* device() const;
        uint channel() const;
        double samplingRate() const;
        const char* units() const;
        virtual double& operator[](int i) = 0;
        virtual const double& operator[](int i) const = 0;
        virtual double& at(int i) = 0;
        virtual const double& at(int i) const = 0;

private:
        char m_device[FILENAME_MAXLEN], m_units[10];
        uint m_channel;
        double m_samplingRate;
};

class ComediChannel : public Channel {
        ComediChannel(const char *device, uint subdevice, uint range, uint reference,
                uint channel, double conversionFactor, double samplingRate, const char *units);
        uint subdevice() const;
        uint range() const;
        uint reference() const;
        double conversionFactor() const;
private:
        uint m_subdevice, m_range, m_reference;
        double m_conversionFactor;
};

class MCSChannel : public Channel {
};

class InputChannel : public ComediChannel {
public:
        InputChannel(const char *device, uint subdevice, uint range, uint reference,
                uint channel, double conversionFactor, double samplingRate, const char *units);
        ~InputChannel();
        const double* data(size_t *length) const;
        bool allocateDataBuffer(double tend);
        double& operator[](int i);
        const double& operator[](int i) const;
        double& at(int i);
        const double& at(int i) const;
private:
        double *m_data;
        size_t m_dataLength;
};

class OutputChannel : public ComediChannel {
public:
        OutputChannel(const char *device, uint subdevice, uint range, uint reference,
                uint channel, double conversionFactor, double samplingRate,
                const char *units, const char *stimfile);
        const char* stimulusFile() const;
        bool setStimulusFile(const char *filename);
        double& operator[](int i);
        const double& operator[](int i) const;
        double& at(int i);
        const double& at(int i) const;
        const Stimulus* stimulus() const;
private:
        Stimulus m_stimulus;
};

struct io_thread_arg {
        io_thread_arg(double final_time, double time_step,
                      std::vector<InputChannel*>* input,
                      const std::vector<OutputChannel*>* output)
                : tend(final_time), dt(time_step),
                  input_channels(input), output_channels(output),
                  nsteps(0) {}
        double tend, dt;
        std::vector<InputChannel*>* input_channels;
        const std::vector<OutputChannel*>* output_channels;
        size_t nsteps;
};

/**
 * This thread performs I/O using the Comedi library. Its features are:
 * 1) It performs both input and output operations.
 * 2) It assumes that all input and output channels are on the same device.
 * 3) It assumes that all input channels and all output channels are on the
 *    same subdevice (which are different between in and out, obviously).
 */
void* io_thread(void *in);

#endif // DAQ_IO_H

