LDADD = ../stimgen/libstimgen.la ../src/liblcg.la
AM_CPPFLAGS = -I../stimgen -I../src -I.
bin_PROGRAMS = lcg lcg-help lcg-experiment lcg-annotate
lcg_SOURCES = lcg.cpp
lcg_experiment_SOURCES = lcg-experiment.cpp
lcg_help_SOURCES = lcg-help.cpp
lcg_annotate_SOURCES = lcg-annotate.cpp
if OUTPUT_RESET
AM_CPPFLAGS += -DRESET_OUTPUT
endif
if REALTIME
AM_CPPFLAGS += -DREALTIME_ENGINE
endif
if ANALOG_IO
AM_CPPFLAGS += -DANALOG_IO
bin_PROGRAMS += lcg-vcclamp lcg-test-daq lcg-output lcg-zero
lcg_vcclamp_SOURCES = lcg-vcclamp.cpp
lcg_test_daq_SOURCES = lcg-test_daq.cpp
lcg_output_SOURCES = lcg-output.cpp
lcg_zero_SOURCES = lcg-zero.cpp
endif
if ANALOGY
noinst_PROGRAMS = analogy_test
analogy_test_SOURCES = analogy_test.cpp
endif
