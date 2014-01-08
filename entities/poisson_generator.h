/*=========================================================================
 *
 *   Program:     lcg
 *   Filename:    poisson_generator.h
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

#ifndef POISSON_GENERATOR_H
#define POISSON_GENERATOR_H

#include "types.h"
#include "utils.h"
#include "randlib.h"
#include "generator.h"

#define POISSON_RATE m_parameters["rate"]

namespace lcg {

namespace generators {

class Poisson : public Generator {
public:
        Poisson(double rate, ullong seed, uint id = GetId());
        virtual bool initialise();
        virtual bool hasNext() const;
        virtual double output();
        virtual void step();

private:
        void calculateTimeNextSpike();

private:
        UniformRandom m_random;
        double m_tNextSpike;
};

} // namespace generators

} // namespace lcg

/***
 *   FACTORY METHODS
 ***/
#ifdef __cplusplus
extern "C" {
#endif

lcg::Entity* PoissonFactory(string_dict& args);
	
#ifdef __cplusplus
}
#endif


#endif

