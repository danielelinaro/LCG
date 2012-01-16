#ifdef HAVE_LIBCOMEDI

#ifndef ANALOG_IO_H
#define ANALOG_IO_H

#include "utils.h"
#include "entity.h"

#define COMEDI_IO_CONV_FACTOR m_parameters[0]

namespace dynclamp {

class ComediAnalogIO {
public:
        ComediAnalogIO(const char *deviceFile, uint subdevice, uint channel, uint id = GetId());
        virtual ~ComediAnalogIO();

        const char* deviceFile() const;
        const uint subdevice() const;
        const uint channel() const;

protected:
        bool openDevice();
        void closeDevice();

protected:
        char m_deviceFile[30];
        comedi_t *m_device;
        uint m_subdevice, channel;
        comedi_range *m_dataRange;
        lsampl_t m_maxData;
};

class ComediAnalogInput : public ComediAnalogIO {
public:
        ComediAnalogInput(const char *deviceFile, uint inputSubdevice,
                          uint readChannel, double inputConversionFactor = 100,
                          uint id = GetId());
        double inputConversionFactor() const;
        double read();
};

class ComediAnalogOutput : public ComediAnalogIO {
public:
        ComediAnalogOutput(const char *deviceFile, uint outputSubdevice,
                           uint writeChannel, double outputConversionFactor = 0.0025,
                           uint id = GetId());
        double outputConversionFactor() const;
        void write(double data);
};

} // namespace dynclamp

#endif // ANALOG_IO_H

#endif // HAVE_LIBCOMEDI
