/*=========================================================================
 *
 *   Program:     lcg
 *   Filename:    periodic_pulse.h
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

#ifndef PERIODIC_PULSE
#define PERIODIC_PULSE

#include "generator.h"

#define PP_FREQUENCY    m_parameters["frequency"]
#define PP_DURATION     m_parameters["duration"]
#define PP_AMPLITUDE    m_parameters["amplitude"]
#define PP_PERIOD       m_parameters["period"]

namespace lcg {

namespace generators {

class PeriodicPulse : public Generator {
public:
        PeriodicPulse(double frequency, double duration, double amplitude, std::string units = "pA", uint id = GetId());

        virtual bool initialise();

        virtual bool hasNext() const;

        virtual void step();
        virtual double output();

        double period() const;

        void setFrequency(double frequency);
        void setPeriod(double period);
        void setAmplitude(double amplitude);
        void setDuration(double duration);

private:
        double m_output;
        double m_tNextPulse;
};

} // namespace generators

} // namespace lcg

/***
 *   FACTORY METHODS
 ***/
#ifdef __cplusplus
extern "C" {
#endif

lcg::Entity* PeriodicPulseFactory(string_dict& args);
        
#ifdef __cplusplus
}
#endif

#endif

