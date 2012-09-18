/*=========================================================================
 *
 *   Program:     dynclamp
 *   Filename:    converter.h
 *
 *   Copyright (C) 2012 Daniele Linaro
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

/*!
 * \file converter.h
 * \brief Definition of the class Converter
 */

#ifndef CONVERTER_H
#define CONVERTER_H

#include <string>
#include "entity.h"

namespace dynclamp {

/*!
 * \class Converter
 * \brief Converts a change in the value of its <b>sole</b> input to a call
 * to the method changeProperty of the <b>sole</b> entity to which it is connected.
 */
class Converter : public Entity {
public:
        Converter(std::string propertyName, uint id = GetId());
        virtual void step();
        virtual double output() const;
        virtual bool initialise();

private:
        std::string m_propertyName;
        bool m_first;
        double m_previousInput;
};

} // namespace dynclamp

#endif

