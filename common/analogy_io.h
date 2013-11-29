#ifndef ANALOGY_IO_H
#define ANALOGY_IO_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string>
#include <map>
#include <analogy/analogy.h>
#include "types.h"
#include "entity.h"
#include "common.h"

namespace lcg {

/**
 * \brief Base class for analog I/O with Analogy.
 */
class AnalogyAnalogIO {
public:
        AnalogyAnalogIO(const char *deviceFile, uint subdevice,
                        uint *channels, uint nChannels,
                        uint range = PLUS_MINUS_TEN,
                        uint aref = GRSE);
        virtual ~AnalogyAnalogIO();

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
        a4l_desc_t m_dsc;
        uint m_subdevice;
        uint *m_channels;
        uint m_nChannels;
        uint m_range;
        uint m_aref;
        double *m_data;
};

/**
 * \brief Class for analog input from a single channel with hardware calibration.
 */
/*
class AnalogyAnalogInputHardCal : public AnalogyAnalogIO {
public:
        AnalogyAnalogInputHardCal(const char *deviceFile, uint outputSubdevice,
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
*/

/**
 * \brief Class for analog output to a single channel with hardware calibration.
 */
/*
class AnalogyAnalogOutputHardCal : public AnalogyAnalogIO {
public:
        AnalogyAnalogOutputHardCal(const char *deviceFile, uint outputSubdevice,
                                  uint writeChannel, double outputConversionFactor,
                                  uint aref = GRSE);
        ~AnalogyAnalogOutputHardCal();
        bool initialise();
        double outputConversionFactor() const;
        void write(double data);
private:
        comedi_range *m_dataRange;
        lsampl_t m_maxData;
        double m_outputConversionFactor;
};
*/

/**
 * \brief Base class for analog I/O with software calibration.
 */
class AnalogyAnalogIOSoftCal : public AnalogyAnalogIO {
public:
        AnalogyAnalogIOSoftCal(const char *deviceFile, uint subdevice,
                               uint *channels, uint nChannels,
                               uint range = PLUS_MINUS_TEN,
                               uint aref = GRSE);
        ~AnalogyAnalogIOSoftCal();

protected:
        bool readCalibration();

protected:
        char *m_calibrationFile;
        //comedi_calibration_t *m_calibration;
};

/**
 * \brief Class for analog input from a single channel with software calibration.
 */
class AnalogyAnalogInputSoftCal : public AnalogyAnalogIOSoftCal {
public:
        AnalogyAnalogInputSoftCal(const char *deviceFile, uint inputSubdevice,
                                  uint readChannel, double inputConversionFactor,
                                  uint range = PLUS_MINUS_TEN,
                                  uint aref = GRSE);
        ~AnalogyAnalogInputSoftCal();
        bool initialise();
        double inputConversionFactor() const;
        double read();
private:
        double m_inputConversionFactor;
        //comedi_polynomial_t m_converter;
};

/**
 * \brief Class for analog output to a single channel with software calibration.
 */
class AnalogyAnalogOutputSoftCal : public AnalogyAnalogIOSoftCal {
public:
        AnalogyAnalogOutputSoftCal(const char *deviceFile, uint outputSubdevice,
                                  uint writeChannel, double outputConversionFactor,
                                  uint aref = GRSE);
        ~AnalogyAnalogOutputSoftCal();
        bool initialise();
        double outputConversionFactor() const;
        void write(double data);
private:
        double m_outputConversionFactor;
        //comedi_polynomial_t m_converter;
};

} // namespace lcg

#endif // ANALOGY_IO_H

