#ifndef CHANNEL_H
#define CHANNEL_H

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <comedilib.h>
#include <time.h>
#include <math.h>

#include <vector>
#include <map>

#include "stimulus.h"
#include "stream.h"

namespace lcg {

class Channel : public Stream {
public:
        Channel(const char *device, uint channel, double samplingRate, const char *units, uint id = GetId());
        virtual ~Channel();
        const char* device() const;
        uint channel() const;
        double samplingRate() const;
        virtual double& operator[](int i) = 0;
        virtual const double& operator[](int i) const = 0;
        virtual double& at(int i) = 0;
        virtual const double& at(int i) const = 0;

private:
        char m_device[FILENAME_MAXLEN];
        uint m_channel;
        double m_samplingRate;
};

class ComediChannel : public Channel {
public:
        ComediChannel(const char *device, uint subdevice, uint range, uint reference,
                uint channel, double conversionFactor, double samplingRate, const char *units, uint id = GetId());
        uint subdevice() const;
        uint range() const;
        uint reference() const;
        double conversionFactor() const;
        virtual bool initialise();
        virtual void terminate();
        virtual void run();
        virtual void join();
private:
        uint m_subdevice, m_range, m_reference;
        double m_conversionFactor;
};

//class MCSChannel : public Channel {
//};

class InputChannel : public ComediChannel {
public:
        InputChannel(const char *device, uint subdevice, uint range, uint reference,
                uint channel, double conversionFactor, double samplingRate, const char *units, uint id = GetId());
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
                const char *units, const char *stimfile, uint id = GetId());
        const char* stimulusFile() const;
        bool setStimulusFile(const char *filename);
        double& operator[](int i);
        const double& operator[](int i) const;
        double& at(int i);
        const double& at(int i) const;
        const Stimulus* stimulus() const;
        virtual bool hasMetadata(size_t *ndims) const;
        virtual const double* metadata(size_t *dims, char *label) const;
private:
        Stimulus m_stimulus;
};

class Device {
public:
        Device(const char *device, bool autoDestroy = true);
        ~Device();
        bool addChannel(ComediChannel *channel);
        bool removeChannel(ComediChannel *channel);
        bool isChannelPresent(ComediChannel *channel) const;
        void acquire();
        void join();
private:
        bool isSameDevice(ComediChannel *channel) const;
private:
        std::string m_device;
        std::map< int,std::map<int,ComediChannel*> > m_subdevices;
        bool m_autoDestroy;
        bool m_acquiring;
};

/*
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
*/
/**
 * This thread performs I/O using the Comedi library. Its features are:
 * 1) It performs both input and output operations.
 * 2) It assumes that all input and output channels are on the same device.
 * 3) It assumes that all input channels and all output channels are on the
 *    same subdevice (which are different between in and out, obviously).
 */
//void* io_thread(void *in);

} // namespace lcg

/***
 *   FACTORY METHODS
 ***/
#ifdef __cplusplus
extern "C" {
#endif

lcg::Stream* InputChannelFactory(string_dict& args);
lcg::Stream* OutputChannelFactory(string_dict& args);
	
#ifdef __cplusplus
}
#endif

#endif // DAQ_IO_H

