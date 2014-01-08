/*=========================================================================
 *
 *   Program:     lcg
 *   Filename:    generator.h
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

#ifndef GENERATOR_H
#define GENERATOR_H

#include "entity.h"

namespace lcg {

namespace generators {

class Generator : public Entity {
public:
        Generator(uint id = GetId()) : Entity(id) {
                setName("Generator");
        }
        virtual ~Generator() {}

        virtual bool hasNext() const = 0;
};

} // namespace generators

} // namespace lcg

#endif

