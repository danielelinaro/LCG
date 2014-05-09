
AC_PREREQ([2.61])

# init autoconf, automake and libtool
AC_INIT([lcg], [0.9.9], [danielelinaro@gmail.com])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([common/utils.h])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
LT_INIT

# operating system's information
AC_CANONICAL_HOST

# required programs
AC_PROG_AWK
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL

# required header files
AC_CHECK_HEADERS([unistd.h])

# C language stuff
AC_C_CONST
AC_C_INLINE

# configuration macros
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_FILES([Makefile
		stimgen/Makefile
		common/Makefile
                entities/Makefile
                streams/Makefile
                engine/Makefile
		bin/Makefile
                lcg-env.sh])

# required functions and types
AC_CHECK_FUNCS([floor pow sqrt])
AC_HEADER_STDBOOL
AC_TYPE_SIGNAL
AC_TYPE_SIZE_T

# required libraries
AC_CHECK_LIB([dl], [dlopen], [], [AC_MSG_ERROR(dl library missing.)])
AC_CHECK_LIB([hdf5], [H5Fcreate], [], [AC_MSG_ERROR([HDF5 library missing.])])
AC_CHECK_LIB([hdf5_hl], [H5LTmake_dataset_double], [], [AC_MSG_ERROR([HDF5 library missing.])])
AC_LANG_CPLUSPLUS
AC_CHECK_LIB([boost_thread], [main], [], [AC_MSG_ERROR([BOOST thread library missing.])])

# defines
LIB_CURRENT=0
LIB_REVISION=0
LIB_AGE=0
LIB_VER=$LIB_CURRENT:$LIB_REVISION:$LIB_AGE
LIB_TAG=$LIB_CURRENT.$LIB_REVISION.$LIB_AGE
AC_SUBST([LIB_VER])
AC_SUBST([LIB_TAG])

AC_ARG_ENABLE([realtime], AS_HELP_STRING([--disable-realtime], [Disable real-time capabilities]))

with_analog_io=no
with_comedi=no
with_analogy=no
with_rt=no
with_rtai=no
with_xenomai=no

if test `expr $host_os : 'linux'` -eq 5 ; then
   
   AC_CHECK_LIB([rt], [clock_gettime], [], [AC_MSG_ERROR([RT library missing.])])

   ### real time ###
   if test "x$enable_realtime" != "xno" ; then
      AC_CHECK_LIB([xenomai], [pthread_create], [], [])
      AC_CHECK_LIB([lxrt], [rt_set_periodic_mode], [], [])
      if test "$ac_cv_lib_xenomai_pthread_create" = "yes" ; then
         with_xenomai=yes
      elif test "$ac_cv_lib_lxrt_rt_set_periodic_mode" = "yes" ; then
         with_rtai=yes
      fi
      with_rt=yes
   fi

   ### analog I/O ###
   AC_CHECK_LIB([rtdm], [rt_dev_open], [], [])
   AC_CHECK_LIB([native], [rt_timer_set_mode], [], [])
   AC_CHECK_LIB([analogy], [a4l_open], [], [])
   if test "$ac_cv_lib_analogy_a4l_open" = "yes" ; then
      with_analogy=yes
      with_analog_io=yes
   fi
   if test "$with_analogy" = "no" ; then
      AC_CHECK_LIB([comedi], [comedi_open], [], [])
      if test "$ac_cv_lib_comedi_comedi_open" = "yes" ; then
      	 with_comedi=yes
      	 with_analog_io=yes
      fi
   fi
fi

AM_CONDITIONAL([ANALOG_IO],[test "$with_analog_io" = "yes"])
AM_CONDITIONAL([COMEDI],[test "$with_comedi" = "yes"])
AM_CONDITIONAL([ANALOGY],[test "$with_analogy" = "yes"])
AM_CONDITIONAL([REALTIME],[test "$with_rt" = "yes"])
AM_CONDITIONAL([RTAI],[test "$with_rtai" = "yes"])
AM_CONDITIONAL([XENOMAI],[test "$with_xenomai" = "yes"])

AC_SUBST(with_rt)
AC_OUTPUT

