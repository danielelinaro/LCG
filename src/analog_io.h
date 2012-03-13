#ifndef ANALOG_IO_H
#define ANALOG_IO_H

#ifdef HAVE_CONFIG_H
#include "config.h"

#ifdef HAVE_LIBCOMEDI
#include <comedilib.h>
#include "types.h"
#include "entity.h"

/** Non-Referenced Single Ended */
#define NRSE AREF_COMMON
/** Ground-Referenced Single Ended */
#define GRSE AREF_GROUND

enum {
        PLUS_MINUS_TEN = 0,
        PLUS_MINUS_FIVE,
        PLUS_MINUS_ONE,
        PLUS_MINUS_ZERO_POINT_TWO
};

namespace dynclamp {

class ComediAnalogIO {
public:
        ComediAnalogIO(const char *deviceFile, uint subdevice, uint channel,
                       uint range = PLUS_MINUS_TEN,
                       uint aref = GRSE);
        virtual ~ComediAnalogIO();

        const char* deviceFile() const;
        uint subdevice() const;
        uint channel() const;
        uint range() const;
        uint reference() const;

protected:
        bool openDevice();

protected:
        char m_deviceFile[30];
        comedi_t *m_device;
        uint m_subdevice;
        uint m_channel;
        uint m_range;
        uint m_aref;

        comedi_range *m_dataRange;
        lsampl_t m_maxData;
};

class ComediAnalogIOSoftCal : public ComediAnalogIO {
public:
        ComediAnalogIOSoftCal(const char *deviceFile, uint subdevice, uint channel,
                              uint range = PLUS_MINUS_TEN,
                              uint aref = GRSE);
        ~ComediAnalogIOSoftCal();

protected:
        bool readCalibration();

protected:
        char *m_calibrationFile;
        comedi_calibration_t *m_calibration;
};

class ComediAnalogInput : public ComediAnalogIO {
public:
        ComediAnalogInput(const char *deviceFile, uint inputSubdevice,
                          uint readChannel, double inputConversionFactor,
                          uint range = PLUS_MINUS_TEN,
                          uint aref = GRSE);
        double inputConversionFactor() const;
        double read();
private:
        double m_inputConversionFactor;
};

class ComediAnalogOutput : public ComediAnalogIO {
public:
        ComediAnalogOutput(const char *deviceFile, uint outputSubdevice,
                           uint writeChannel, double outputConversionFactor,
                           uint aref = GRSE);
        ~ComediAnalogOutput();
        double outputConversionFactor() const;
        void write(double data);
private:
        double m_outputConversionFactor;
};

class ComediAnalogInputSoftCal : public ComediAnalogIOSoftCal {
public:
        ComediAnalogInputSoftCal(const char *deviceFile, uint inputSubdevice,
                                 uint readChannel, double inputConversionFactor,
                                 uint range = PLUS_MINUS_TEN,
                                 uint aref = GRSE);
        double inputConversionFactor() const;
        double read();
private:
        comedi_polynomial_t m_converter;
        double m_inputConversionFactor;
};

class ComediAnalogOutputSoftCal : public ComediAnalogIOSoftCal {
public:
        ComediAnalogOutputSoftCal(const char *deviceFile, uint outputSubdevice,
                                  uint writeChannel, double outputConversionFactor,
                                  uint aref = GRSE);
        ~ComediAnalogOutputSoftCal();
        double outputConversionFactor() const;
        void write(double data);
private:
        comedi_polynomial_t m_converter;
        double m_outputConversionFactor;
};

class AnalogInput : public Entity {
public:
        AnalogInput(const char *deviceFile, uint inputSubdevice,
                    uint readChannel, double inputConversionFactor,
                    uint range = PLUS_MINUS_TEN,
                    uint aref = GRSE,
                    uint id = GetId());
        virtual void step();
        virtual double output() const;
private:
        double m_data;
        ComediAnalogInputSoftCal m_input;
};

class AnalogOutput : public Entity {
public:
        AnalogOutput(const char *deviceFile, uint outputSubdevice,
                     uint writeChannel, double outputConversionFactor,
                     uint aref = GRSE,
                     uint id = GetId());
        ~AnalogOutput();
        virtual void step();
        virtual double output() const;
private:
        double m_data;
        ComediAnalogOutputSoftCal m_output;
};

} // namespace dynclamp

/***
 *   FACTORY METHODS
 ***/
#ifdef __cplusplus
extern "C" {
#endif

dynclamp::Entity* AnalogInputFactory(dictionary& args);
dynclamp::Entity* AnalogOutputFactory(dictionary& args);
	
#ifdef __cplusplus
}
#endif

#endif // HAVE_LIBCOMEDI

#endif // HAVE_CONFIG_H

#endif // ANALOG_IO_H

