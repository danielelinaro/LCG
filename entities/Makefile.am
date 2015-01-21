AM_CPPFLAGS = -I@top_srcdir@/stimgen -I@top_srcdir@/common -I@top_srcdir@/engine
lib_LTLIBRARIES = liblcg_entities.la
liblcg_entities_la_SOURCES = entity.cpp dynamical_entity.cpp synapses.cpp neurons.cpp poisson_generator.cpp waveform.cpp recorders.cpp periodic_pulse.cpp currents.cpp delay.cpp conductance_stimulus.cpp trigger.cpp pid.cpp frequency_estimator.cpp event_counter.cpp connections.cpp functors.cpp constants.cpp converter.cpp probability_estimator.cpp events.cpp ou.cpp plot.cpp
liblcg_entities_la_LDFLAGS = -version-info ${LIB_VER}
include_HEADERS = entity.h dynamical_entity.h synapses.h neurons.h waveform.h poisson_generator.h recorders.h periodic_pulse.h currents.h delay.h conductance_stimulus.h trigger.h pid.h frequency_estimator.h event_counter.h connections.h functors.h constants.h converter.h probability_estimator.h events.h ou.h generator.h plot.h
if REALTIME
AM_CPPFLAGS += -DREALTIME_ENGINE
endif
if ANALOG_IO
AM_CPPFLAGS += -DANALOG_IO
liblcg_entities_la_SOURCES += analog_io.cpp digital_io.cpp
include_HEADERS += analog_io.h digital_io.h
endif

