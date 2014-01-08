/*=========================================================================
 *
 *   Program:     lcg
 *   Filename:    stimulus.h
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

#ifndef STIMULI_H
#define STIMULI_H

#include "types.h"
#include "common.h"

class Stimulus {
public:
        Stimulus(double dt = -1., const char *filename = NULL);
        virtual ~Stimulus();

        const char* stimulusFile() const;
        bool setStimulusFile(const char *filename);
        void setDt(double dt);
        double dt() const;

        const double* data(size_t *length) const;
        const double* metadata(size_t *rows, size_t *cols) const;
        double& operator[](int i);
        const double& operator[](int i) const;
        double& at(int i);
        const double& at(int i) const;

        size_t length() const;
        double duration() const;

private:
        bool parseStimulusFile();
        void freeMemory();

private:
        char m_filename[FILENAME_MAXLEN];
        double m_dt;
        double *m_stimulus, *m_metadata;
        size_t m_length, m_metadataRows, m_metadataCols;
};

#endif

