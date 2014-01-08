/*=========================================================================
 *
 *   Program:     lcg
 *   Filename:    trigger.h
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

#ifndef TRIGGER_H
#define TRIGGER_H

#include "entity.h"
#include "utils.h"

namespace lcg {

class Trigger : public Entity {
public:
        Trigger(uint id = GetId());
        virtual double output();
protected:
        void emitTrigger() const;
};

#define PT_FREQUENCY m_parameters["f"]

class PeriodicTrigger : public Trigger {
public:
        PeriodicTrigger(double frequency, uint id = GetId());

        void setFrequency(double frequency);
        double period() const;
        void setPeriod(double period);

        virtual bool initialise();
        virtual void step();
private:
        double m_period;
        double m_tNextTrigger;
};

} // namespace lcg

/***
 *   FACTORY METHODS
 ***/
#ifdef __cplusplus
extern "C" {
#endif

lcg::Entity* PeriodicTriggerFactory(string_dict& args);
        
#ifdef __cplusplus
}
#endif


#endif

