/*=========================================================================
 *
 *   Program:     lcg
 *   Filename:    digital_io.h
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

#ifndef DIGITAL_IO_H

#define DIGITAL_IO_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string>
#if defined(HAVE_LIBCOMEDI)
#include "comedi_io.h"
#endif
namespace lcg {

class DigitalInput : public Entity {
public:
        DigitalInput(const char *deviceFile, uint inputSubdevice,
                    uint readChannel,
                    const std::string& units = "Boolean",
                    EventType eventToSend = TRIGGER, uint id = GetId());
        virtual bool initialise();
        virtual void step();
        virtual void firstStep();
        virtual double output();
private:
        double m_data;
		double m_previous;
        EventType m_eventToSend;
#if defined(HAVE_LIBCOMEDI)
        ComediDigitalInput m_input;
#endif
};

class DigitalOutput : public Entity {
public:
        DigitalOutput(const char *deviceFile, uint outputSubdevice,
                     uint writeChannel,
                     const std::string& units = "Boolean",
                     EventType eventToSend = TRIGGER, uint id = GetId());
        ~DigitalOutput();
        virtual bool initialise();
        virtual void terminate();
        virtual void step();
        virtual void firstStep();
        virtual double output();
private:
        double m_data;
        EventType m_eventToSend;
#if defined(HAVE_LIBCOMEDI)
        ComediDigitalOutput m_output;
#endif
};

} // namespace lcg

/***
 *   FACTORY METHODS
 ***/
#ifdef __cplusplus
extern "C" {
#endif

lcg::Entity* DigitalInputFactory(string_dict& args);
lcg::Entity* DigitalOutputFactory(string_dict& args);
	
#ifdef __cplusplus
}
#endif

#endif // ANALOG_IO_H

