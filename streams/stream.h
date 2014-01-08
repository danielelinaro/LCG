/*=========================================================================
 *
 *   Program:     lcg
 *   Filename:    stream.h
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

/*!
 * \file stream.h
 * \brief Definition of the class Stream
 */

#ifndef STREAM_H
#define STREAM_H

#include <stdio.h>
#include "types.h"
#include "utils.h"

#if defined(__APPLE__)
#define STREAMS_LIBNAME "liblcg_streams.dylib"
#elif defined(__linux__)
#define STREAMS_LIBNAME "liblcg_streams.so"
#endif

namespace lcg
{

class Stream {
public:
        /*!
         * Costructor that simply assigns an identifier to the stream.
         * \param id The identifier of the stream.
         */
        Stream(uint id = GetId());

        /*! Destructor: calls the terminate method on the stream. */
        virtual ~Stream();

        /*! Returns the identifier of this stream. */
        uint id() const;

        /*! Returns the number of parameters of this stream. */
        size_t numberOfParameters() const;

        /*! Returns the string_dict with the parameters. */
        const double_dict& parameters() const;

        /*! 
         * Returns a reference to the parameter with a given name.
         * Throws an exception if no parameter with such name exists.
         */
        double& parameter(const std::string name);

        /*!
         * Connects this stream to the one passed as a parameter,
         * i.e., the stream passed as a parameter will have access to the data of this stream.
         */
        void connect(Stream* stream);

        /*! Returns a vector that contains all the streams connected to this stream. */
        const std::vector<Stream*>& pre() const;

        /*! Returns the name of the stream. */
        const std::string& name() const;

        /*!
         * Returns the units of measure of the output of this stream. Such units should
         * be set in the constructor of the derived class. The default value for the units is N/A.
         */
        const std::string& units() const;

        /*!
         * Performs required initialisations of the stream. This method is called before
         * the experiment/simulation starts.
         * \return true if the initialisation was successfull, false otherwise.
         */
        virtual bool initialise() = 0;

        /*!
         * Performs (optionally) required clean-up of the stream, after the experiment/simulation
         * has finished. This is not an abstract method, because the default behaviour is to
         * do nothing to clean-up the stream.
         */
        virtual void terminate();

        /*!
         * Should return true if the stream contains additional metadata, i.e. a multidimensional array
         * of doubles that characterise in some way the behaviour or the output of the stream. The default
         * implementation returns false.
         * \param ndims Used to inform the caller of the number of dimensions of the metadata array.
         * \sa Waveform for an example of how to use metadata.
         */
        virtual bool hasMetadata(size_t *ndims) const;

        /*!
         * Should return a pointer to the metadata contained in this stream. The number of dimensions
         * of the metadata can be obtained by calling hasMetadata.
         * \param dims An array of size ndims (obtained with a call to hasMetadata) that contains
         *             the dimensions of the metadata array.
         * \param label A string that describes the metadata.
         */
        virtual const double* metadata(size_t *dims, char *label) const;

        /*! Provides access to the whole data buffer. */
        virtual const double* data(size_t *length) const = 0;

        /*! Provides access to the i-th element in the internal buffer of this stream. */
        virtual double& operator[](int i) = 0;

        /*! Provides access to the i-th element in the internal buffer of this stream. */
        virtual const double& operator[](int i) const = 0;

        /*! Provides access to the i-th element in the internal buffer of this stream. */
        virtual double& at(int i) = 0;

        /*! Provides access to the i-th element in the internal buffer of this stream. */
        virtual const double& at(int i) const = 0;

        /*! Instructs the stream to perform its operations, for the duration specified by tend. */
        virtual void run(double tend) = 0;

        /*! Blocks until the stream is running. err = 0 means that everything went fine. */
        virtual void join(int *err) = 0;

protected:
        /*!
         * Adds a stream to the list of objects that provide inputs to this stream.
         * \param stream A stream to be connected to this stream.
         */
        virtual void addPre(Stream *stream);

        /*! Sets the name of this stream. */
        void setName(const std::string& name);

        /*! Sets the units of measure of the output of this stream. */
        void setUnits(const std::string &units);

private:
        /*! Checks whether this stream is connected to the stream passed as a parameter of the method. */
        bool isPost(const Stream *stream) const;

protected:
        /*! The numerical identifier of this stream. */
        uint   m_id;

        /*! The parameters of this stream. */
        double_dict m_parameters;

        /*! The vector of streams that provide inputs to this stream. */
        std::vector<Stream*> m_pre;

private:
        /*! The name of this stream. */
        std::string m_name;

        /*! The units of measure of the output of this stream. */
        std::string m_units;
};

/*!
 * \class StreamSorter
 * \brief A helper class to sort streams in a list according to their ID.
 */
class StreamSorter {
public:
    bool operator() (const Stream* s1, const Stream* s2) { return s1->id() < s2->id(); }
};

Stream* StreamFactory(const char *name, string_dict& args);

}

#endif

