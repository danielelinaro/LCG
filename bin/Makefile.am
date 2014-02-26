LDADD = ../common/liblcg_common.la ../stimgen/liblcg_stimgen.la ../entities/liblcg_entities.la ../engine/liblcg_engine.la ../streams/liblcg_streams.la
AM_CPPFLAGS = -I@top_srcdir@/stimgen -I@top_srcdir@/common -I@top_srcdir@/entities -I@top_srcdir@/streams -I@top_srcdir@/engine
bin_PROGRAMS = lcg lcg-help lcg-experiment lcg-annotate
lcg_SOURCES = lcg.cpp
lcg_experiment_SOURCES = lcg-experiment.cpp
lcg_help_SOURCES = lcg-help.cpp
lcg_annotate_SOURCES = lcg-annotate.cpp
if REALTIME
AM_CPPFLAGS += -DREALTIME_ENGINE
endif
if ANALOG_IO
AM_CPPFLAGS += -DANALOG_IO
bin_PROGRAMS += lcg-vcclamp lcg-test-daq lcg-output lcg-zero
lcg_vcclamp_SOURCES = lcg-vcclamp.cpp
lcg_test_daq_SOURCES = lcg-test-daq.cpp
lcg_output_SOURCES = lcg-output.cpp
lcg_zero_SOURCES = lcg-zero.cpp
if NONRT
bin_PROGRAMS += lcg-non-rt
lcg_non_rt_SOURCES = lcg-non-rt.cpp 
endif
if ANALOGY
noinst_PROGRAMS = analogy_test
analogy_test_SOURCES = analogy_test.cpp
endif
endif
