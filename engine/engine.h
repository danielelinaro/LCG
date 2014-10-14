/*=========================================================================
 *
 *   Program:     lcg
 *   Filename:    engine.h
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

#ifndef ENGINE_H
#define ENGINE_H

#include "types.h"
#include "utils.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifdef HAVE_LIBLXRT
#include <rtai_lxrt.h>

/** Convert internal count units to milliseconds */
#define count2ms(t)     ((double) count2nano(t)*1e-6)
/** Convert milliseconds to internal count units*/
#define ms2count(t)     nano2count((t)*1e6)
/** Convert internal count units to seconds */
#define count2sec(t)     ((double) count2nano(t)*1e-9)
/** Convert seconds to internal count units */
#define sec2count(t)     nano2count((t)*1e9)

#endif // HAVE_LIBLXRT

namespace lcg {

class Entity;
class Stream;

struct trigger_data {
        trigger_data(const char *device = "/dev/comedi0", uint subdevice = 0, 
			uint channel = 0, double threshold = 2.5, uint range = 0, uint aref = 0x00)  
                : use(false), device(device), subdevice(subdevice),
		channel(channel), threshold(threshold),
		range(range), aref(aref) {}
        bool use;
	const char *device;
	uint subdevice;
	uint channel;
	double threshold;
	uint range;
	uint aref;
};

int Simulate(std::vector<Entity*> *entities, double tend, struct trigger_data trigger);
int Simulate(std::vector<Stream*> *streams, double tend, const std::string& outfilename);

#ifdef REALTIME_ENGINE
extern double globalTimeOffset;
#define GetGlobalTimeOffset() globalTimeOffset
#endif

#ifdef HAVE_LIBLXRT
extern double realtimeDt;
//#define NO_STOP_RT_TIMER
#undef GetGlobalDt
#define GetGlobalDt() realtimeDt
#undef IncreaseGlobalTime
#define IncreaseGlobalTime() (globalT += realtimeDt)
#define SetGlobalTimeOffset() (globalTimeOffset = count2sec(rt_get_time()))
#endif // HAVE_LIBLXRT

#ifdef HAVE_LIBANALOGY
#include <sys/mman.h>
#define SetGlobalTimeOffset() (globalTimeOffset = ((double) rt_timer_ticks2ns(rt_timer_read())) / NSEC_PER_SEC)
#endif // HAVE_LIBANALOGY

#ifdef REALTIME_ENGINE
#define SCHEDULER SCHED_RR
#define SetGlobalTimeOffset(now) (globalTimeOffset = now.tv_sec + ((double) now.tv_nsec / NSEC_PER_SEC))
#endif // REALTIME_ENGINE

} // namespace lcg

#endif // ENGINE_H

