/*=========================================================================
 *
 *   Program:     lcg
 *   Filename:    aec.h
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
 * \file aec.h
 * \brief Definition of the class AEC
 */

#ifndef AEC_H
#define AEC_H

#include <stddef.h>

namespace lcg {

/*!
 * \class AEC
 * \brief This class implements the active electrode compensation.
 *
 * The purpose of this class is to perform, at each time step, a compensation 
 * of the membrane voltage of a recorded neuron, given the previous values
 * of the injected current. This allows the usage of one single electrode
 * for performing dynamic clamp, without having to use the discontinuous current
 * clamp mode of the amplifier and without having to apply neither bridge-balancing
 * nor capacitance compensation.
 *
 * The technique is described in detail in the following two papers:
 *
 * Brette, R., Piwkowska, Z., Rudolph, M., Bal, T., & Destexhe, A. (2007).<br/>
 * <b>A non-parametric electrode model for intracellular recording</b>.<br/>
 * <i>Neurocomputing</i>, 70(10), 1597–1601.
 *
 * Brette, R., Piwkowska, Z., Monier, C., Rudolph-Lilith, M., Fournier, J., Levy, M., Frégnac, Y., et al. (2008).<br/>
 * <b>High-resolution intracellular recordings using a real-time computational model of the electrode</b>.<br/>
 * <i>Neuron</i>, 59(3), 379–391. doi:10.1016/j.neuron.2008.06.021
 *
 * \par
 * \note
 * The AEC class is not an Entity, since it is used internally by neurons::RealNeuron.
 *
 * \par
 * \note
 * The units of measure <b>must</b> be the following:
 *  - electrode kernel: Ohm
 *  - current: pA
 *  - voltage: mV
 */
class AEC {
public:
        /*!
         * Constructs an AEC object and reads the electrode kernel from the file passed as a parameter.
         * Such file should contain only the values of the electrode kernel, either in a sinle line
         * or in multiple lines.
         * The current buffer is initialised with the value of 0.
         * \param kernelFile The file where the electrode kernel (units of measure: Ohm) is stored. If kernelFile is NULL,
         * no electrode kernel is read and calls to compensate will simply return the value of voltage
         * passed as argument.
         */
        AEC(const char *kernelFile = NULL);

        /*!
         * Constructs an AEC object, using the kernel passed as argument.
         * The current buffer is initialised with the value of 0.
         * \param kernel The electrode kernel to be used for compensation.
         * \param kernelSize The number of samples of the electrode kernel.
         */
        AEC(const double *kernel, size_t kernelSize);

        /*! Destroys the AEC object. */
        ~AEC();

        /*!
         * Initializes the current buffer and the membrane voltage with the
         * values passed as a parameter.
         * \param I The current value to be used to fill the current buffer (in pA).
         * \param V The previous value of the membrane voltage (in mV).
         */
        virtual bool initialise(double I = 0, double V = 0);

        /*!
         * Initializes the current buffer and the membrane voltage with the
         * values passed as a parameter.
         * \param I An array with the values of current that should be used
         * to fill the internal current buffer (in pA). It should have the same
         * length of the electrode kernel.
         * \param V The previous value of the membrane voltage (in mV).
         */
        virtual bool initialise(double *I, double V = 0);

        /*! Returns true if an electrode kernel has been provided. */
        bool hasKernel() const;

        /*!
         * Adds a new value of current to the current buffer: this function
         * should be called at each time step.
         * \param I The last value of current that was injected into a neuron.
         * \sa neurons::RealNeuron
         */
        void pushBack(double I);

        double compensate(double V);

        size_t kernelLength() const;
        const double* kernel() const;

private:
        double convolve();

private:
        size_t m_length;
        unsigned int m_pos;
        double *m_kernel;
        double *m_current;
        double m_buffer[2];
        bool m_withKernel;
};

} // namespace lcg

#endif

