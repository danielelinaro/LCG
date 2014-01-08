/*=========================================================================
 *
 *   Program:     lcg
 *   Filename:    waveform.h
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

#ifndef WAVEFORM_H
#define WAVEFORM_H

#include "entity.h"
#include "types.h"
#include "utils.h"
#include "generator.h"
#include "common.h"
#include "stimulus.h"

namespace lcg {

namespace generators {

class Waveform : public Generator {
public:
        Waveform(const char *stimulusFile = NULL, bool triggered = false,
                 const std::string& units = "N/A", uint id = GetId());
        virtual ~Waveform();

        const char* stimulusFile() const;
        bool setStimulusFile(const char *filename);
        virtual bool initialise();

        uint stimulusLength() const;
        virtual bool hasNext() const;

        virtual void step();
        virtual void terminate();

        virtual double output(); 
        virtual void handleEvent(const Event *event);
        virtual bool hasMetadata(size_t *ndims) const;
        virtual const double* metadata(size_t *dims, char *label) const;

        double duration() const;

protected:
        virtual void reset();

private:
        uint m_position;
        bool m_triggered;
        bool m_eventSent;
        Stimulus *m_stimulus;
        char m_stimulusFile[FILENAME_MAXLEN];
};

} // namespace generators

} // namespace lcg

/***
 *   FACTORY METHODS
 ***/
#ifdef __cplusplus
extern "C" {
#endif

lcg::Entity* WaveformFactory(string_dict& args);
	
#ifdef __cplusplus
}
#endif

#endif

