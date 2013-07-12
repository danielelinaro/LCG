AM_CPPFLAGS = -I../stimgen -I../entities -I.
lib_LTLIBRARIES = liblcg_common.la
liblcg_common_la_SOURCES = randlib.cpp utils.cpp aec.cpp sha1.cpp stimuli.cpp configuration.cpp h5rec.cpp
liblcg_common_la_LDFLAGS = -version-info ${LIB_VER}
include_HEADERS = types.h randlib.h utils.h thread_safe_queue.h aec.h common.h sha1.h stimuli.h configuration.h h5rec.h
if OUTPUT_RESET
AM_CPPFLAGS += -DRESET_OUTPUT
endif
if ANALOG_IO
AM_CPPFLAGS += -DANALOG_IO
if COMEDI
liblcg_common_la_SOURCES += daq_io.cpp comedi_io.cpp
include_HEADERS += daq_io.h comedi_io.h
endif
if ANALOGY
liblcg_common_la_SOURCES += analogy_io.cpp
include_HEADERS += analogy_io.h
endif
endif
