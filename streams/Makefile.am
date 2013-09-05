AM_CPPFLAGS = -I../stimgen -I../common -I.
lib_LTLIBRARIES = liblcg_streams.la
liblcg_streams_la_SOURCES = stream.cpp
liblcg_streams_la_LDFLAGS = -version-info ${LIB_VER}
include_HEADERS = stream.h
if OUTPUT_RESET
AM_CPPFLAGS += -DRESET_OUTPUT
endif
if ANALOG_IO
AM_CPPFLAGS += -DANALOG_IO
#if COMEDI
#liblcg_streams_la_SOURCES += daq_io.cpp comedi_io.cpp configuration.cpp
#include_HEADERS += daq_io.h comedi_io.h configuration.h
#endif
#if ANALOGY
#liblcg_streams_la_SOURCES += analogy_io.cpp
#include_HEADERS += analogy_io.h
#endif
endif
