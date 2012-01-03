LDADD = ../stimgen/libstimgen.la ../src/libdynclamp.la
AM_CPPFLAGS = -I../stimgen -I../src -I.

if EXAMPLES
noinst_PROGRAMS = ou_test syn_test lif_test stimgen_test small_network
ou_test_SOURCES = ou_test.cpp
syn_test_SOURCES = syn_test.cpp
lif_test_SOURCES = lif_test.cpp
stimgen_test_SOURCES = stimgen_test.cpp
small_network_SOURCES = small_network.cpp
endif

