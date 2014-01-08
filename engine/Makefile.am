AM_CPPFLAGS = -I@top_srcdir@/stimgen -I@top_srcdir@/common -I@top_srcdir@/entities -I@top_srcdir@/streams
lib_LTLIBRARIES = liblcg_engine.la
liblcg_engine_la_SOURCES = engine.cpp
liblcg_engine_la_LDFLAGS = -version-info ${LIB_VER}
include_HEADERS = engine.h
if REALTIME
AM_CPPFLAGS += -DREALTIME_ENGINE
endif
if ANALOG_IO
AM_CPPFLAGS += -DANALOG_IO
endif
