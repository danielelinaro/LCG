/*=========================================================================
 *
 *   Program:     lcg
 *   Filename:    converter.h
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

/*!
 * \file converter.h
 * \brief Definition of the class Converter
 */

#ifndef CONVERTER_H
#define CONVERTER_H

#include <string>
#include "entity.h"

namespace lcg {

/*!
 * \class Converter
 * \brief Converts a change in the value of its <b>sole</b> input to a call
 * to the ``parameter'' method of the <b>sole</b> entity to which it is connected.
 */
class Converter : public Entity {
public:
        /*!
         * Constructor.
         * \param parameterName The name of the parameter of the post entity to be changed whenever
         * the input to this Converter changes.
         * \param id The id of this object.
         */
        Converter(std::string parameterName, uint id = GetId());

        /*!
         * This method compares the current input to the previous one. In case they are
         * different, the actual value is used to change the value of one of the parameters
         * of the post entity.
         */
        virtual void step();

        /*! This method only outputs 0. */
        virtual double output();

        /*! Performs required initialisation. */
        virtual bool initialise();

private:
        /*! The name of the parameter in the post entity to change whenever a change in the input is detected. */
        std::string m_parameterName;
        /* This flag is true only on the first step, when no operation should be performed. */
        bool m_first;
        /*! The value of the input at the previous step. */
        double m_previousInput;
};

} // namespace lcg

/***
 *   FACTORY METHODS
 ***/
#ifdef __cplusplus
extern "C" {
#endif

lcg::Entity* ConverterFactory(string_dict& args);
        
#ifdef __cplusplus
}
#endif

#endif

