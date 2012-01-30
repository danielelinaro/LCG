AM_CPPFLAGS = -I../stimgen -I. -DDEBUG
lib_LTLIBRARIES = libdynclamp.la
libdynclamp_la_SOURCES = randlib.cpp utils.cpp entity.cpp dynamical_entity.cpp ou.cpp synapses.cpp events.cpp neurons.cpp poisson_generator.cpp stimulus_generator.cpp recorders.cpp aec.cpp analog_io.cpp engine.cpp periodic_pulse.cpp
libdynclamp_la_LDFLAGS = -version-info ${LIB_VER}
include_HEADERS = types.h randlib.h utils.h entity.h dynamical_entity.h ou.h synapses.h events.h thread_safe_queue.h neurons.h stimulus_generator.h poisson_generator.h recorders.h aec.h analog_io.h engine.h periodic_pulse.h
