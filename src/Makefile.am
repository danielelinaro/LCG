AM_CPPFLAGS = -I../stimgen -I.
lib_LTLIBRARIES = liblcg.la
liblcg_la_SOURCES = randlib.cpp utils.cpp entity.cpp dynamical_entity.cpp synapses.cpp events.cpp neurons.cpp poisson_generator.cpp waveform.cpp recorders.cpp aec.cpp engine.cpp periodic_pulse.cpp currents.cpp delay.cpp conductance_stimulus.cpp trigger.cpp pid.cpp frequency_estimator.cpp event_counter.cpp connections.cpp functors.cpp constants.cpp converter.cpp probability_estimator.cpp sha1.cpp stimuli.cpp configuration.cpp h5rec.cpp
liblcg_la_LDFLAGS = -version-info ${LIB_VER}
include_HEADERS = types.h randlib.h utils.h entity.h dynamical_entity.h synapses.h events.h thread_safe_queue.h neurons.h waveform.h poisson_generator.h recorders.h aec.h engine.h periodic_pulse.h currents.h delay.h conductance_stimulus.h trigger.h pid.h frequency_estimator.h event_counter.h connections.h functors.h constants.h common.h converter.h probability_estimator.h sha1.h stimuli.h configuration.h h5rec.h
if OUTPUT_RESET
AM_CPPFLAGS += -DRESET_OUTPUT
endif
if ASYNCHRONOUS_IO
AM_CPPFLAGS += -DASYNCHRONOUS_IO
endif
if REALTIME
AM_CPPFLAGS += -DREALTIME_ENGINE
endif
if ANALOG_IO
AM_CPPFLAGS += -DANALOG_IO
liblcg_la_SOURCES += analog_io.cpp
include_HEADERS += analog_io.h
if COMEDI
liblcg_la_SOURCES += comedi_io.cpp daq_io.cpp
include_HEADERS += comedi_io.h daq_io.h
endif
if ANALOGY
liblcg_la_SOURCES += analogy_io.cpp
include_HEADERS += analogy_io.h
endif
endif

