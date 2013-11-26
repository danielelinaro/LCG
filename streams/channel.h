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
        virtual void run(double tend);
        virtual void join(int *err);
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
        virtual void run(double tend);
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
        const double* data(size_t *length) const;
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
        void acquire(double tend);
        void join(int *err);
        size_t numberOfChannels() const;
private:
        bool isSameDevice(ComediChannel *channel) const;
        static void* IOThread(void *arg);
private:
        char m_device[FILENAME_MAXLEN];
        std::map< int,std::map<int,ComediChannel*> > m_subdevices;
        bool m_autoDestroy, m_acquiring, m_joined;
        int m_err;
        size_t m_numberOfChannels;
        double m_tend;
        pthread_t m_ioThread;
};

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

