AM_CPPFLAGS = -I../stimgen -I../common -I../entities -I../streams -I.
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
