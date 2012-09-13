/*=========================================================================
 *
 *   Program:     dynclamp
 *   Filename:    entity.h
 *
 *   Copyright (C) 2012 Daniele Linaro
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
 * \file entity.h
 * \brief Definition of the class Entity
 */

#ifndef ENTITY_H
#define ENTITY_H

#include <vector>
#include <string>

#include "types.h"
#include "utils.h"
#include "events.h"

namespace dynclamp
{

class Entity;

/*!
 * \class Entity
 * \brief Base abstract class for (almost) all objects in dynclamp
 *
 * Entities are the basic building blocks of experiments and/or simulations
 * performed with dynclamp. The main characteristics of an Entity are the
 * following:
 *  - it has multiple inputs but <b>only one</b> output. This simplifies
 *    significantly the connectivity between entities, but can also result
 *    in some drawbacks.
 *  - an entity has a unique, non-negative identfier.
 *  - at every time step, an entity performs some task.
 *  - an entity may have a certain number of parameters, which influence its
 *    behaviour.
 *  - entities can communicate in an asynchronous fashion, by sending events.
 *  - all entities in an experiment/simulation are updated in parallel, which
 *    means that first all entities read their inputs from connected entities
 *    and store them and then compute their output. This prevents problems
 *    related to the way entities are actually connected.
 *
 * Every new class added to dynclamp should inherit from Entity, in order to
 * be simulated properly.
 */
class Entity
{
public:
        /*!
         * Costructor that simply assigns an identifier to the entity.
         * \param id The identifier of the entity.
         */
        Entity(uint id = GetId());

        /*! Destructor: calls the terminate method on the entity. */
        virtual ~Entity();

        /*! Returns the identifier of this entity. */
        uint id() const;

        /*!
         * Sets the parameters of this entity.
         * \param parameters A vector of double precision floating point values,
         * which must contain at least as many elements as the number of parameters
         * of the entity.
         */
        void setParameters(const array& parameters);

        /*!
         * Sets the value of the i-th parameter of this entity.
         * \param parameter The parameter.
         * \param index The index of the parameter in the parameters' array. If this
         * value is greater or equal to the number of parameters of this entity, an
         * exception is thrown.
         */
        void setParameter(double parameter, uint index);

        /*!
         * Sets the name of the parameters. This is useful for describing parameters
         * with easy-to-remember labels.
         * \param parametersNames A vector with the names of the parameters, as string objects.
         */
        void setParametersNames(const strings& parametersNames);

        /*!
         * Sets the name of the i-th parameter.
         * \param parameterName The name of the parameter.
         * \param index The index of the parameter in the parameters' array. If this
         * value is greater or equal to the number of parameters of this entity, an
         * exception is thrown.
         */
        void setParameterName(const std::string& parameterName, uint index);

        /*! Returns the number of parameters of this entity. */
        size_t numberOfParameters() const;

        /*! Returns the array with the parameters. */
        const array& parameters() const;

        /*!
         * Returns the i-th parameter.
         * \param index The index of the parameter in the parameters' array. If this
         * value is greater or equal to the number of parameters of this entity, an
         * exception is thrown.
         */
        double parameter(uint index) const;

        /*! Returns the names of all parameters. */
        const strings& parametersNames() const;

        /*! Returns the name of the i-th parameter. */
        const std::string& parameterName(uint index) const;

        /**
         * Connects this entity to the one passed as a parameter,
         * i.e., this entity will be an input of the one passed
         * as a parameter.
         * \param entity The entity to which this entity will be connected as an input.
         */
        void connect(Entity* entity);

        /*! Returns a vector that contains all the entities connected to this entity. */
        const std::vector<Entity*>& pre() const;

        /*! Returns a vector that contains all the entities this entity is connected to. */
        const std::vector<Entity*>& post() const;

        /*!
         * Calling this method causes the entity to read the output values of all the
         * entities connected to it and store it for future use.
         */
        void readAndStoreInputs();

        /*! Returns the name of the entity. */
        const std::string& name() const;

        /*!
         * Returns the units of measure of the output of this entity. Such units should
         * be set in the constructor of the derived class. The default value for the units is N/A.
         */
        const std::string& units() const;

        /*!
         * This method is called at each iteration of the experiment/simulation.
         * Derived classes should implement here the code for the evolution of the entity.
         */
        virtual void step() = 0;

        /*! Returns the output value of this entity. */
        virtual double output() const = 0;

        /*!
         * Performs required initialisations of the entity. This method is called before
         * the experiment/simulation starts.
         * \return true if the initialisation was successfull, false otherwise.
         */
        virtual bool initialise() = 0;

        /*!
         * Performs (optionally) required clean-up of the entity, after the experiment/simulation
         * has finished. This is not an abstract method, because the default behaviour is to
         * do nothing to clean-up the entity.
         */
        virtual void terminate();

        /*!
         * This method is called when the entity receives an event sent by another entity.
         * \param event The event to handle.
         */
        virtual void handleEvent(const Event *event);

        /*!
         * This method is used for emitting events that have this entity as sender.
         * \param event The event to send to all the entities connected.
         */
        virtual void emitEvent(Event *event) const;

        /*!
         * Should return true if the entity contains additional metadata, i.e. a multidimensional array
         * of doubles that characterise in some way the behaviour or the output of the entity. The default
         * implementation returns false.
         * \param ndims Used to inform the caller of the number of dimensions of the metadata array.
         * \sa Waveform for an example of how to use metadata.
         */
        virtual bool hasMetadata(size_t *ndims) const;

        /*!
         * Should return a pointer to the metadata contained in this entity. The number of dimensions
         * of the metadata can be obtained by calling hasMetadata.
         * \param dims An array of size ndims (obtained with a call to hasMetadata) that contains
         *             the dimensions of the metadata array.
         * \param label A string that describes the metadata.
         */
        virtual const double* metadata(size_t *dims, char *label) const;

protected:
        /*!
         * Adds an entity to the list of objects that provide inputs to this entity.
         * \param entity An entity to be connected to this entity.
         */
        virtual void addPre(Entity *entity);

        /*!
         * Adds an entity to the list of objects this entity is connected to.
         * \param entity An entity this entity will be connected to.
         */
        virtual void addPost(Entity *entity);

        /*! Sets the name of this entity. */
        void setName(const std::string& name);

        /*! Sets the units of measure of the output of this entity. */
        void setUnits(const std::string &units);

private:
        /*! Checks whether this entity is connected to the entity passed as a parameter of the method. */
        bool isPost(const Entity *entity) const;

protected:
        /*! The numerical identifier of this entity. */
        uint   m_id;

        /*! The array where the inputs to this entity will be stored. */
        array  m_inputs;

        /*! The array of parameters of this entity. */
        array  m_parameters;

        /*! The names of the parameters. */
        strings m_parametersNames;

        /*! The vector of entities that provide inputs to this entity. */
        std::vector<Entity*> m_pre;

        /*! The vector of entities this entity is connected to. */
        std::vector<Entity*> m_post;

private:
        /*! The name of this entity. */
        std::string m_name;

        /*! The units of measure of the output of this entity. */
        std::string m_units;
};

/*!
 * \class EntitySorter
 * \brief A helper class to sort entities in a list according to their ID.
 */
class EntitySorter {
public:
    bool operator() (const Entity* e1, const Entity* e2) { return e1->id() < e2->id(); }
};

}

#endif

