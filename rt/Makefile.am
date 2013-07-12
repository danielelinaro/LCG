AM_CPPFLAGS = -I../stimgen -I../common -I../entities -I.
lib_LTLIBRARIES = liblcg_rt.la
liblcg_rt_la_SOURCES = engine.cpp
liblcg_rt_la_LDFLAGS = -version-info ${LIB_VER}
include_HEADERS = engine.h
if ANALOG_IO
AM_CPPFLAGS += -DANALOG_IO
endif
