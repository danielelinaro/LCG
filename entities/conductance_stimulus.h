/*=========================================================================
 *
 *   Program:     lcg
 *   Filename:    conductance_stimulus.h
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

#ifndef CONDUCTANCE_STIMULUS_H
#define CONDUCTANCE_STIMULUS_H

#include "types.h"
#include "utils.h"
#include "entity.h"
#include "generator.h"
#include "neurons.h"

#define COND_E  m_parameters["E"]
#define NMDA_COND_K1 m_parameters["K1"]
#define NMDA_COND_K2 m_parameters["K2"]

namespace lcg {

namespace generators {

class ConductanceStimulus : public Generator {
public:
        ConductanceStimulus(double E, uint id = GetId());
        virtual bool initialise();
        virtual bool hasNext() const;
        virtual void step();
        virtual double output();
protected:
        virtual void addPost(Entity *entity);
protected:
        double m_output;
        neurons::Neuron *m_neuron;
};

class NMDAConductanceStimulus : public ConductanceStimulus {
public:
        NMDAConductanceStimulus(double E, double K1, double K2, uint id = GetId());
        virtual void step();
};

} // namespace lcg

} // namespace generators

/***
 *   FACTORY METHODS
 ***/
#ifdef __cplusplus
extern "C" {
#endif

lcg::Entity* ConductanceStimulusFactory(string_dict& args);
lcg::Entity* NMDAConductanceStimulusFactory(string_dict& args);
	
#ifdef __cplusplus
}
#endif

#endif

