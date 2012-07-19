#ifndef ANALOG_IO_H
#define ANALOG_IO_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_LIBCOMEDI

#include <string>
#include "comedi_io.h"

namespace dynclamp {

class AnalogInput : public Entity {
public:
        AnalogInput(const char *deviceFile, uint inputSubdevice,
                    uint readChannel, double inputConversionFactor,
                    uint range = PLUS_MINUS_TEN,
                    uint aref = GRSE,
                    const std::string& units = "mV",
                    uint id = GetId());
        virtual bool initialise();
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
                    const std::string& units = "pA",
                     uint id = GetId());
        ~AnalogOutput();
        virtual bool initialise();
        virtual void terminate();
        virtual void step();
        virtual double output() const;
private:
        double m_data;
        ComediAnalogOutputSoftCal m_output;
};

class AnalogIO : public Entity {
public:
        AnalogIO(const char *deviceFile, uint inputSubdevice,
                 uint readChannel, double inputConversionFactor,
                 uint outputSubdevice, uint writeChannel, double outputConversionFactor,
                 uint inputRange = PLUS_MINUS_TEN, uint aref = GRSE,
                 const std::string& units = "mV",
                 uint id = GetId());
        ~AnalogIO();
        virtual bool initialise();
        virtual void terminate();
        virtual void step();
        virtual double output() const;
private:
        double m_data;
        ComediAnalogInputSoftCal m_input;
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
dynclamp::Entity* AnalogIOFactory(dictionary& args);
	
#ifdef __cplusplus
}
#endif

#endif // HAVE_LIBCOMEDI

#endif // ANALOG_IO_H

