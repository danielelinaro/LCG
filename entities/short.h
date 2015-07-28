/*=========================================================================
 *
 *   Program:     lcg
 *   Filename:    short.h
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

#ifndef SHORT_H
#define SHORT_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string>
#if defined(HAVE_LIBCOMEDI)
#include "comedi_io.h"
#elif defined(HAVE_LIBANALOGY)
#include "analogy_io.h"
#endif

namespace lcg {

class Short : public Entity {
public:
        Short(const char *deviceFile, uint inputSubdevice,
              uint outputSubdevice, uint readChannel,
              uint writeChannel, double inputConversionFactor,
              double outputConversionFactor,
              const std::string& inputUnits = "mV",
              const std::string& outputUnits = "pA",
              uint inputRange = PLUS_MINUS_TEN,
              uint reference = GRSE, bool resetOutput = false,
              uint id = GetId());
        ~Short();
        virtual bool initialise();
        virtual void terminate();
        virtual void step();
        virtual double output();
private:
        double m_data;
        bool m_resetOutput;
#if defined(HAVE_LIBCOMEDI)
        ComediAnalogInputSoftCal m_input;
        ComediAnalogOutputSoftCal m_output;
#elif defined(HAVE_LIBANALOGY)
        AnalogyAnalogIO m_input;
        AnalogyAnalogIO m_output;
#endif
};

} // namespace lcg

/***
 *   FACTORY METHODS
 ***/
#ifdef __cplusplus
extern "C" {
#endif

lcg::Entity* ShortFactory(string_dict& args);
	
#ifdef __cplusplus
}
#endif

#endif // ANALOG_IO_H

