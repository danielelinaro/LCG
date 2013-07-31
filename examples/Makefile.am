LDADD = ../stimgen/liblcg_stimgen.la ../common/liblcg_common.la
AM_CPPFLAGS = -I../stimgen -I../common -I.
if OUTPUT_RESET
AM_CPPFLAGS += -DRESET_OUTPUT
endif
if REALTIME
AM_CPPFLAGS += -DREALTIME_ENGINE
endif
noinst_PROGRAMS = aec_test
aec_test_SOURCES = aec_test.cpp

