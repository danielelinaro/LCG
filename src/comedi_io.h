#ifndef COMEDI_IO_H
#define COMEDI_IO_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_LIBCOMEDI
#include <string>
#include <map>
#include <comedilib.h>
#include "types.h"
#include "entity.h"
#include "common.h"

/** Non-Referenced Single Ended */
#define NRSE AREF_COMMON

/** Ground-Referenced Single Ended */
#define GRSE AREF_GROUND

/**
 * We ask the card to operate at a frequency
 * of OVERSAMPLING_FACTOR times the sampling rate.
 */
#define OVERSAMPLING_FACTOR 1

namespace dynclamp {

/**
 * \brief Base class for analog I/O with Comedi.
 */
class ComediAnalogIO {
public:
        ComediAnalogIO(const char *deviceFile, uint subdevice,
                       uint *channels, uint nChannels,
                       uint range = PLUS_MINUS_TEN,
                       uint aref = GRSE);
        virtual ~ComediAnalogIO();

        virtual bool addChannel(uint channel);
        bool isChannelPresent(uint channel);

        const char* deviceFile() const;
        uint subdevice() const;
        const uint* channels() const;
        uint nChannels() const;
        uint range() const;
        uint reference() const;

protected:
        bool openDevice();
        bool closeDevice();

protected:
        char m_deviceFile[30];
        comedi_t *m_device;
        uint m_subdevice;
        uint *m_channels;
        uint m_nChannels;
        uint m_range;
        uint m_aref;
        lsampl_t *m_data;
};

#define PROXY_BUFSIZE 1024

/**
 * \brief Proxy class for analog I/O on multiple channels.
 */
class ComediAnalogIOProxy : public ComediAnalogIO {
public:
        ComediAnalogIOProxy(const char *deviceFile, uint subdevice,
                            uint *channels, uint nChannels,
                            uint range = PLUS_MINUS_TEN,
                            uint aref = GRSE);
        ~ComediAnalogIOProxy();

        bool initialise();

        // analog input
        void acquire();
        lsampl_t value(uint channel);

        // analog output
        void output(uint channel, lsampl_t value);

        void increaseRefCount();
        void decreaseRefCount();
        uint refCount() const;

private:
        bool stopCommand();
        void packChannelsList();
        bool issueCommand();
        bool fixCommand();
        bool startCommand();

private:
        char m_buffer[PROXY_BUFSIZE];
        comedi_cmd m_cmd;
        bool m_commandRunning;
        double m_tLastSample;
        uint m_refCount;
        uint *m_channelsPacked;
        uint m_bytesPerSample;
        int m_subdeviceFlags;
        int m_deviceFd;

        // key -> channel number, value -> sample
        std::map<uint,lsampl_t> m_hash;

        /*** used only for analog output ***/
        int m_bytesToWrite, m_nStoredSamples;

        /*** used only for analog input ***/
        int m_bytesToRead;
};

/**
 * \brief Base class for analog I/O with software calibration.
 */
class ComediAnalogIOSoftCal : public ComediAnalogIO {
public:
        ComediAnalogIOSoftCal(const char *deviceFile, uint subdevice,
                              uint *channels, uint nChannels,
                              uint range = PLUS_MINUS_TEN,
                              uint aref = GRSE);
        ~ComediAnalogIOSoftCal();

protected:
        bool readCalibration();

protected:
        char *m_calibrationFile;
        comedi_calibration_t *m_calibration;
};

/**
 * \brief Class for analog input from a single channel with software calibration.
 */
class ComediAnalogInputSoftCal : public ComediAnalogIOSoftCal {
public:
        ComediAnalogInputSoftCal(const char *deviceFile, uint inputSubdevice,
                                 uint readChannel, double inputConversionFactor,
                                 uint range = PLUS_MINUS_TEN,
                                 uint aref = GRSE);
        ~ComediAnalogInputSoftCal();
        bool initialise();
        double inputConversionFactor() const;
        double read();
private:
#ifdef ASYNCHRONOUS_IO
        ComediAnalogIOProxy *m_proxy;
#endif
        comedi_polynomial_t m_converter;
        double m_inputConversionFactor;
};

/**
 * \brief Class for analog output to a single channel with software calibration.
 */
class ComediAnalogOutputSoftCal : public ComediAnalogIOSoftCal {
public:
        ComediAnalogOutputSoftCal(const char *deviceFile, uint outputSubdevice,
                                  uint writeChannel, double outputConversionFactor,
                                  uint aref = GRSE);
        ~ComediAnalogOutputSoftCal();
        bool initialise();
        double outputConversionFactor() const;
        void write(double data);
private:
#ifdef ASYNCHRONOUS_IO
        ComediAnalogIOProxy *m_proxy;
#endif
        comedi_polynomial_t m_converter;
#ifdef TRIM_ANALOG_OUTPUT
        comedi_range *m_dataRange;
#endif
        double m_outputConversionFactor;
};

/**
 * \brief Class for analog input from a single channel with hardware calibration.
 */
class ComediAnalogInputHardCal : public ComediAnalogIO {
public:
        ComediAnalogInputHardCal(const char *deviceFile, uint outputSubdevice,
                                 uint readChannel, double inputConversionFactor,
                                 uint range = PLUS_MINUS_TEN, uint aref = GRSE);
        bool initialise();
        double inputConversionFactor() const;
        double read();
private:
        comedi_range *m_dataRange;
        lsampl_t m_maxData;
        double m_inputConversionFactor;
};

/**
 * \brief Class for analog output to a single channel with hardware calibration.
 */
class ComediAnalogOutputHardCal : public ComediAnalogIO {
public:
        ComediAnalogOutputHardCal(const char *deviceFile, uint outputSubdevice,
                                  uint writeChannel, double outputConversionFactor,
                                  uint aref = GRSE);
        ~ComediAnalogOutputHardCal();
        bool initialise();
        double outputConversionFactor() const;
        void write(double data);
private:
        comedi_range *m_dataRange;
        lsampl_t m_maxData;
        double m_outputConversionFactor;
};

} // namespace dynclamp

#endif // HAVE_LIBCOMEDI

#endif // COMEDI_IO_H

