LDADD = ../src/libdynclamp.la

# debug off
#AM_CPPFLAGS = -I$(srcdir)/../src -I.
AM_CPPFLAGS = -I../src -I.

if EXAMPLES
noinst_PROGRAMS = ou_test syn_test lif_test
ou_test_SOURCES = ou_test.cpp
syn_test_SOURCES = syn_test.cpp
lif_test_SOURCES = lif_test.cpp
endif

