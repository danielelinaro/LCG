#ifndef ANALOG_IO_H
#define ANALOG_IO_H

#ifdef HAVE_CONFIG_H
#include "config.h"

#ifdef HAVE_LIBCOMEDI
#include <comedilib.h>
#include "types.h"
#include "entity.h"

#define RANGE 0

namespace dynclamp {

class ComediAnalogIO {
public:
        ComediAnalogIO(const char *deviceFile, uint subdevice, uint channel);
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
        uint m_subdevice, m_channel;
        comedi_range *m_dataRange;
        lsampl_t m_maxData;
};

class ComediAnalogInput : public ComediAnalogIO {
public:
        ComediAnalogInput(const char *deviceFile, uint inputSubdevice,
                          uint readChannel, double inputConversionFactor);
        double inputConversionFactor() const;
        double read();
private:
        double m_inputConversionFactor;
};

class ComediAnalogOutput : public ComediAnalogIO {
public:
        ComediAnalogOutput(const char *deviceFile, uint outputSubdevice,
                           uint writeChannel, double outputConversionFactor);
        ~ComediAnalogOutput();
        double outputConversionFactor() const;
        void write(double data);
private:
        double m_outputConversionFactor;
};

class AnalogInput : public Entity {
public:
        AnalogInput(const char *deviceFile, uint inputSubdevice,
                    uint readChannel, double inputConversionFactor,
                     uint id = GetId(), double dt = GetGlobalDt());
        virtual void step();
        virtual double output() const;
private:
        double m_data;
        ComediAnalogInput m_input;
};

class AnalogOutput : public Entity {
public:
        AnalogOutput(const char *deviceFile, uint outputSubdevice,
                     uint writeChannel, double outputConversionFactor,
                     uint id = GetId(), double dt = GetGlobalDt());
        virtual void step();
        virtual double output() const;
private:
        double m_data;
        ComediAnalogOutput m_output;
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

