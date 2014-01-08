/*=========================================================================
 *
 *   Program:     lcg
 *   Filename:    probability_estimator.h
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

#ifndef PROBABILITY_ESTIMATOR_H
#define PROBABILITY_ESTIMATOR_H

#include "entity.h"
#include "utils.h"

#define PE_TAU  m_parameters["tau"]
#define PE_P0   m_parameters["p0"]
#define PE_WNDW m_parameters["window"]
#define PE_F    m_parameters["frequency"]
#define PE_T    m_parameters["period"]

namespace lcg {

class ProbabilityEstimator : public Entity {
public:
        ProbabilityEstimator(double tau, double stimulationFrequency, double window,
                        double initialProbability = 0.5, uint id = GetId());
        virtual bool initialise();
        virtual void step();
        virtual double output();
        virtual void handleEvent(const Event *event);
protected:
        void emitTrigger() const;
private:
        double m_tPrevStim;
        double m_tPrevSpike;
        double m_delay;
        double m_probability;
        bool m_flag;
};

} // namespace lcg

/***
 *   FACTORY METHODS
 ***/
#ifdef __cplusplus
extern "C" {
#endif

lcg::Entity* ProbabilityEstimatorFactory(string_dict& args);
        
#ifdef __cplusplus
}
#endif

#endif

