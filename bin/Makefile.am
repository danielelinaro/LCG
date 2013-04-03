LDADD = ../stimgen/libstimgen.la ../src/libdynclamp.la
AM_CPPFLAGS = -I../stimgen -I../src -I.
bin_PROGRAMS = dclamp
dclamp_SOURCES = dclamp.cpp
if OUTPUT_RESET
AM_CPPFLAGS += -DRESET_OUTPUT
endif
if REALTIME
AM_CPPFLAGS += -DREALTIME_ENGINE
endif
if ANALOG_IO
AM_CPPFLAGS += -DANALOG_IO
bin_PROGRAMS += cclamp zero_output constant_output test_daq spont_rec
cclamp_SOURCES = cclamp.cpp
zero_output_SOURCES = zero_output.cpp
constant_output_SOURCES = constant_output.cpp
test_daq_SOURCES = test_daq.cpp
spont_rec_SOURCES = spont_rec.cpp

endif
if ANALOGY
noinst_PROGRAMS = analogy_test
analogy_test_SOURCES = analogy_test.cpp
endif

