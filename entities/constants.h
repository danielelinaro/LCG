/*=========================================================================
 *
 *   Program:     lcg
 *   Filename:    constants.h
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

#ifndef CONSTANTS_H
#define CONSTANTS_H

#include "entity.h"
#include "common.h"

namespace lcg
{

class Constant : public Entity
{
public:
        Constant(double value, const std::string& units = "N/A", uint id = GetId());
        void setValue(double value);
        virtual void step();
        virtual double output();
        virtual bool initialise();
};

class ConstantFromFile : public Constant
{
public:
        ConstantFromFile(const std::string& filename = LOGFILE,
                         const std::string& units = "N/A",
                         uint id = GetId());
        virtual bool initialise();
        std::string filename() const;
        void setFilename(const std::string& filename);
private:
        std::string m_filename;
};

}

/***
 *   FACTORY METHODS
 ***/
#ifdef __cplusplus
extern "C" {
#endif

lcg::Entity* ConstantFactory(string_dict& args);
lcg::Entity* ConstantFromFileFactory(string_dict& args);
	
#ifdef __cplusplus
}
#endif

#endif

