/*=========================================================================
 *
 *   Program:     lcg
 *   Filename:    channel.h
 *
 *   Copyright (C) 2012,2013,2014 Daniele Linaro
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *   
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *   
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *=========================================================================*/

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

class ComediDevice;

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

        friend class ComediDevice;

protected:
        // the length of the VALID data
        size_t m_validDataLength;

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
        // the REAL size of m_data
        size_t m_dataLength;
        double *m_data;
};

class OutputChannel : public ComediChannel {
public:
        OutputChannel(const char *device, uint subdevice, uint range, uint reference,
                uint channel, double conversionFactor, double samplingRate,
                const char *units, const char *stimfile, double offset = 0.,
                bool resetOutput = false, uint id = GetId());
        virtual void terminate();
        const char* stimulusFile() const;
        bool setStimulusFile(const char *filename);
        const double* data(size_t *length) const;
        double& operator[](int i);
        const double& operator[](int i) const;
        double& at(int i);
        const double& at(int i) const;
        const Stimulus* stimulus() const;
        double offset() const;
        virtual bool hasMetadata(size_t *ndims) const;
        virtual const double* metadata(size_t *dims, char *label) const;
private:
        Stimulus m_stimulus;
        double m_offset;
        bool m_resetOutput;
};

class ComediDevice {
public:
        ComediDevice(const char *device, bool autoDestroy = true);
        ~ComediDevice();
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

