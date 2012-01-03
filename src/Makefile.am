AM_CPPFLAGS = -I../stimgen -I. -DDEBUG
lib_LTLIBRARIES = libdynclamp.la
libdynclamp_la_SOURCES = utils.cpp entity.cpp dynamical_entity.cpp ou.cpp synapses.cpp events.cpp neurons.cpp stimulus_generator.cpp
libdynclamp_la_LDFLAGS = -version-info ${LIB_VER}
include_HEADERS = types.h randlib.h utils.h entity.h dynamical_entity.h ou.h synapses.h events.h thread_safe_queue.h neurons.h stimulus_generator.h
