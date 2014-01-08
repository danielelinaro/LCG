/*=========================================================================
 *
 *   Program:     lcg
 *   Filename:    dynamical_entity.h
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

#ifndef DYNAMICAL_ENTITY_H
#define DYNAMICAL_ENTITY_H

#include "entity.h"
#include "types.h"
#include "utils.h"

namespace lcg
{

class DynamicalEntity;

class DynamicalEntity : public Entity
{
public:
        DynamicalEntity(uint id = GetId());

        const array& state() const;

        virtual void step();

protected:
        virtual void evolve() = 0;

protected:
        array  m_state;
};

}

#endif

