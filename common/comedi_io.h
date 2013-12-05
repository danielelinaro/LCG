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

namespace lcg {

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
                                  uint aref = GRSE, bool resetOutput = true);
        ~ComediAnalogOutputSoftCal();
        bool initialise();
        double outputConversionFactor() const;
        void write(double data);
private:
        comedi_polynomial_t m_converter;
#ifdef TRIM_ANALOG_OUTPUT
        comedi_range *m_dataRange;
#endif
        double m_outputConversionFactor;
        bool m_resetOutput;
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
                                  uint aref = GRSE, bool resetOutput = true);
        ~ComediAnalogOutputHardCal();
        bool initialise();
        double outputConversionFactor() const;
        void write(double data);
private:
        comedi_range *m_dataRange;
        lsampl_t m_maxData;
        double m_outputConversionFactor;
        bool m_resetOutput;
};

} // namespace lcg

#endif // HAVE_LIBCOMEDI

#endif // COMEDI_IO_H

