AM_CPPFLAGS = -I../stimgen -I. -DDEBUG
lib_LTLIBRARIES = libdynclamp.la
libdynclamp_la_SOURCES = utils.cpp dynamical_entity.cpp ou.cpp synapses.cpp events.cpp neurons.cpp
libdynclamp_la_LDFLAGS = -version-info ${LIB_VER}
include_HEADERS = types.h randlib.h utils.h dynamical_entity.h ou.h synapses.h events.h thread_safe_queue.h neurons.h
