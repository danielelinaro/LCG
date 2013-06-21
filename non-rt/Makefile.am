LDADD = ../stimgen/libstimgen.la ../src/liblcg.la
AM_CPPFLAGS = -I../stimgen -I../src -I.
bin_PROGRAMS = lcg-non-rt
lcg_non_rt_SOURCES = lcg-non-rt.cpp stimuli.cpp configuration.cpp
if ANALOG_IO
AM_CPPFLAGS += -DANALOG_IO
if OUTPUT_RESET
AM_CPPFLAGS += -DRESET_OUTPUT
endif
endif
