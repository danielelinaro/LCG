/*=========================================================================
 *
 *   Program:     lcg
 *   Filename:    connection.h
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

#ifndef CONNECTION_H
#define CONNECTION_H

#include "entity.h"
#include "functors.h"
#include <list>

namespace lcg {

class Connection : public Entity
{
public:
        Connection(double delay, uint id = GetId());
        ~Connection();
        
        void setDelay(double delay);

        virtual void step();
        virtual double output();
        virtual bool initialise();
        virtual void terminate();
        virtual void handleEvent(const Event *event);

protected:
        virtual void deliverEvent(const Event *event);
        void clearEventsList();

protected:
        std::list< std::pair<double, Event*> > m_events;
};

class SynapticConnection : public Connection
{
public:
        SynapticConnection(double delay, double weight, uint id = GetId());
        void setWeight(double weight);
protected:
        virtual void deliverEvent(const Event *event);
};

class VariableDelayConnection : public Connection
{
public:
        VariableDelayConnection(uint id = GetId());
        virtual void handleEvent(const Event *event);
protected:
        virtual void addPre(Entity *entity);
private:
        Functor *m_functor;
};

}

/***
 *   FACTORY METHODS
 ***/
#ifdef __cplusplus
extern "C" {
#endif

lcg::Entity* ConnectionFactory(string_dict& args);
lcg::Entity* SynapticConnectionFactory(string_dict& args);
lcg::Entity* VariableDelayConnectionFactory(string_dict& args);
        
#ifdef __cplusplus
}
#endif

#endif

