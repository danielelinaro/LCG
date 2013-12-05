AM_CPPFLAGS = -I../stimgen -I../common -I../entities -I.
lib_LTLIBRARIES = liblcg_streams.la
liblcg_streams_la_SOURCES = stream.cpp
liblcg_streams_la_LDFLAGS = -version-info ${LIB_VER}
include_HEADERS = stream.h
if ANALOG_IO
AM_CPPFLAGS += -DANALOG_IO
if COMEDI
liblcg_streams_la_SOURCES += channel.cpp
include_HEADERS += channel.h
endif
#if ANALOGY
#liblcg_streams_la_SOURCES += analogy_io.cpp
#include_HEADERS += analogy_io.h
#endif
endif
