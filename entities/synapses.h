/*=========================================================================
 *
 *   Program:     lcg
 *   Filename:    synapses.h
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

#ifndef SYNAPSES_H
#define SYNAPSES_H

#include <queue>
#include <math.h>
#include <stdio.h>
#include "dynamical_entity.h"

#define SYN_G m_state[0]
#define SYN_E m_parameters["E"]

namespace lcg {

namespace neurons {
class Neuron;
} // namespace neurons

namespace synapses {

class Synapse : public DynamicalEntity {
public:
	Synapse(double E, uint id = GetId());
        virtual bool initialise();
	
        double g() const;
        virtual double output();
        
        /*!
         * A Synapse ignores all events that are not spikes delivered to it.
         */
        virtual void handleEvent(const Event *event);

        /*!
         * Called by a Connection object when a spike is delivered to a Synapse.
         */
        virtual void handleSpike(double weight) = 0;

protected:
        virtual void addPost(Entity *entity);

protected:
        std::deque<double> m_spikeTimeouts;
        double m_tPrevSpike;

private:
        neurons::Neuron *m_neuron;
};

//~~

#define EXP_SYN_DECAY m_parameters["decay_time_constant"]

class ExponentialSynapse : public Synapse {
public:
	ExponentialSynapse(double E, double tau, uint id = GetId());
        virtual bool initialise();
	virtual void handleSpike(double weight);
protected:
	virtual void evolve();	
};

//~~

#define EXP2_SYN_TAU1   m_parameters["tau_1"]
#define EXP2_SYN_TAU2   m_parameters["tau_2"]
#define EXP2_SYN_DECAY1 m_parameters["decay_time_constant_1"]
#define EXP2_SYN_DECAY2 m_parameters["decay_time_constant_2"]
#define EXP2_SYN_FACTOR m_parameters["factor"]

class Exp2Synapse : public Synapse {
public:
	Exp2Synapse(double E, double tau[2], uint id = GetId());
        virtual bool initialise();
	virtual void handleSpike(double weight);
protected:
	virtual void evolve();	
};


//~~

#define TMG_SYN_U                       m_parameters["U"]
#define TMG_SYN_TAU_1                   m_parameters["tau_1"]
#define TMG_SYN_TAU_REC                 m_parameters["tau_rec"]
#define TMG_SYN_TAU_FACIL               m_parameters["tau_facil"]
#define TMG_SYN_DECAY                   m_parameters["decay_time_constant"]
#define TMG_SYN_ONE_OVER_TAU_1		m_parameters["tau_1_recipr"]
#define TMG_SYN_ONE_OVER_TAU_REC	m_parameters["tau_rec_recipr"]
#define TMG_SYN_ONE_OVER_TAU_FACIL	m_parameters["tau_facil_recipr"]
#define TMG_SYN_COEFF                   m_parameters["coeff"]

class TMGSynapse : public Synapse {
public:
	TMGSynapse(double E, double U, double tau[3], uint id = GetId());
        virtual bool initialise();
	virtual void handleSpike(double weight);
protected:
	virtual void evolve();	
};

} // namespace synapses

} // namespace lcg

/***
 *   FACTORY METHODS
 ***/
#ifdef __cplusplus
extern "C" {
#endif

lcg::Entity* ExponentialSynapseFactory(string_dict& args);
lcg::Entity* Exp2SynapseFactory(string_dict& args);
lcg::Entity* TMGSynapseFactory(string_dict& args);
	
#ifdef __cplusplus
}
#endif


#endif
