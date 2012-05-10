AM_CPPFLAGS = -I../stimgen -I.
lib_LTLIBRARIES = libdynclamp.la
libdynclamp_la_SOURCES = randlib.cpp utils.cpp entity.cpp dynamical_entity.cpp ou.cpp synapses.cpp events.cpp neurons.cpp poisson_generator.cpp stimulus_generator.cpp recorders.cpp aec.cpp engine.cpp periodic_pulse.cpp currents.cpp frequency_clamp.cpp delay.cpp time_logger.cpp conductance_stimulus.cpp trigger.cpp pid.cpp frequency_estimator.cpp event_counter.cpp
libdynclamp_la_LDFLAGS = -version-info ${LIB_VER}
include_HEADERS = types.h randlib.h utils.h entity.h dynamical_entity.h ou.h synapses.h events.h thread_safe_queue.h neurons.h stimulus_generator.h poisson_generator.h recorders.h aec.h engine.h periodic_pulse.h currents.h frequency_clamp.h delay.h time_logger.h conductance_stimulus.h trigger.h pid.h frequency_estimator.h event_counter.h
if ANALOG_IO
libdynclamp_la_SOURCES += comedi_io.cpp analog_io.cpp
include_HEADERS += comedi_io.h analog_io.h
endif

