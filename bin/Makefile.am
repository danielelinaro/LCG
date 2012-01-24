LDADD = ../stimgen/libstimgen.la ../src/libdynclamp.la
AM_CPPFLAGS = -I../stimgen -I../src -I.

if EXAMPLES
noinst_PROGRAMS = ou_test syn_test lif_test stimgen_test poisson_test h5rec_test aec_test dynamic_load_test small_network autapse
bin_PROGRAMS = afs_clone
ou_test_SOURCES = ou_test.cpp
syn_test_SOURCES = syn_test.cpp
lif_test_SOURCES = lif_test.cpp
stimgen_test_SOURCES = stimgen_test.cpp
poisson_test_SOURCES = poisson_test.cpp
h5rec_test_SOURCES = h5rec_test.cpp
aec_test_SOURCES = aec_test.cpp
dynamic_load_test_SOURCES = dynamic_load_test.cpp
small_network_SOURCES = small_network.cpp
autapse_SOURCES = autapse.cpp
afs_clone_SOURCES = afs_clone.cpp
endif

