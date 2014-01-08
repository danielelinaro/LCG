/*=========================================================================
 *
 *   Program:     lcg
 *   Filename:    delay.h
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

#ifndef DELAY_H
#define DELAY_H

#include "entity.h"
#include "utils.h"

namespace lcg {

class Delay : public Entity {
public:
        Delay(uint nSamples = 1, uint id = GetId());
        Delay(double delay, uint id = GetId());
        virtual ~Delay();

        virtual bool initialise();
        virtual void step();
        virtual double output();

private:
        void allocateBuffer();

private:
        uint m_bufferLength;
        double *m_buffer;
        uint m_bufferPosition;
};

} // namespace dinclamp

/***
 *   FACTORY METHODS
 ***/
#ifdef __cplusplus
extern "C" {
#endif

lcg::Entity* DelayFactory(string_dict& args);
	
#ifdef __cplusplus
}
#endif

#endif

