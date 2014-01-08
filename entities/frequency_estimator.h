/*=========================================================================
 *
 *   Program:     lcg
 *   Filename:    frequency_estimator.h
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

#ifndef FREQUENCY_ESTIMATOR_H
#define FREQUENCY_ESTIMATOR_H

#include "entity.h"
#include "utils.h"

#define FE_TAU m_parameters["tau"]
#define FE_F0  m_parameters["f0"]

namespace lcg {

class FrequencyEstimator : public Entity {
public:
        FrequencyEstimator(double tau, double initialFrequency = 0.0, uint id = GetId());
        void setTau(double tau);
        void changeState();
	bool state();
        virtual bool initialise();
        virtual void step();
        virtual double output();
        virtual void handleEvent(const Event *event);
protected:
        void emitTrigger() const;
private:
	bool m_state;
        double m_tPrevSpike;
        double m_frequency;
};

} // namespace lcg

/***
 *   FACTORY METHODS
 ***/
#ifdef __cplusplus
extern "C" {
#endif

lcg::Entity* FrequencyEstimatorFactory(string_dict& args);
        
#ifdef __cplusplus
}
#endif

#endif

