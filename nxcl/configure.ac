dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.59c)
AC_INIT([nxcl], [1.0], [seb@esfnet.co.uk])
AC_CONFIG_SRCDIR(nxcl/nxcl.cpp)
AC_REVISION([$Revision: 1.3 $])
AC_PREFIX_DEFAULT(/usr/local)

AM_INIT_AUTOMAKE([1.10 foreign])

AM_CONFIG_HEADER(config.h)
AM_MAINTAINER_MODE

AC_LANG_CPLUSPLUS

dnl Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LIBTOOL

dnl check for boost library
dnl I would prefer to include boost code and compile in place and statically link.
dnl AC_CHECK_LIB(boost_signals, LIB_BOOST_SIGNALS="-lboost_signals")
dnl AC_SUBST(LIB_BOOST_SIGNALS)

AC_ARG_WITH(nxcmd, 
        AC_HELP_STRING([--with-nxcmd], [build nxcmd]),
        [with_nxcmd=${withval}],
        [with_nxcmd=auto])

AS_IF([test "$with_nxcmd" != no], [
        PKG_CHECK_MODULES(DBUS, [
                dbus-1
                ],
                [with_nxcmd=yes],
                [
                AS_IF([test "$with_nxcmd" = yes], [
                        AC_MSG_ERROR([nxcmd support was requested but D-Bus libraries are not available])
               ])

               with_nxcmd=no
               ])
])

AM_CONDITIONAL(WITH_NXCMD, test "$with_nxcmd" = yes)

AC_ARG_WITH(doxygen,
            AC_HELP_STRING([--with-doxygen], [build doxygen API documentation]),
        [with_doxygen=${withval}],
        [with_doxygen=yes])

if test "$with_doxygen" = yes; then
  AC_CHECK_PROG(DOXYGEN, doxygen, doxygen, /bin/echo)
fi

AM_CONDITIONAL(WITH_DOXYGEN, test "$with_doxygen" = yes)

dnl Checks for library functions.
AC_HEADER_STDC
AC_HEADER_TIME
AC_CHECK_FUNCS(strftime gettimeofday uname)

dnl Determine host system type
AC_CANONICAL_HOST
AC_DEFINE_UNQUOTED(HOST, "$host", [The host system nxcl was configured for])

dnl pkg_modules="libxml2" <- Check this and modify when necessary
dnl PKG_CHECK_MODULES(PACKAGE, [$pkg_modules])
AC_SUBST(PACKAGE_CFLAGS)
AC_SUBST(PACKAGE_LIBS)

dnl enable/disable compiler warnings (using gcc), if specified
AC_ARG_ENABLE(warnings,
  [  --disable-warnings      disable gcc warnings [default=no]],
  [case "${enableval}" in
    yes) nxcl_warnings=yes ;; 
    no)  nxcl_warnings=no ;;
    *) AC_MSG_ERROR(bad value ${enableval} for --enable-warnings) ;;
  esac], nxcl_warnings=yes)
AC_MSG_CHECKING(whether to use gcc verbose warnings)
AC_MSG_RESULT($nxcl_warnings)
if (test "$nxcl_warnings" = yes && test "$GCC" = yes); then
  CXXFLAGS="-Wall $CXXFLAGS"
fi

dnl enable/disable debug logging, if specified
AC_ARG_ENABLE(debug-output,
  [  --enable-debug-output  enable debug output [default=no]],
  [case "${enableval}" in
    yes) nxcl_debug=yes ;;
    no)  nxcl_debug=no ;;
    *) AC_MSG_ERROR(bad value ${enableval} for --enable-debug-output) ;;
  esac], nxcl_debug=no)
AC_MSG_CHECKING(whether to enable debugging output)
AC_MSG_RESULT($nxcl_debug)

AM_CONDITIONAL(DEBUG, test "$nxcl_debug" = yes)

dnl Do some platform checking for cygwin

if test "$(uname -o)" = Cygwin; then
  AC_DEFINE(NXCL_CYGWIN, 1, Define if compiling on Cygwin)
else
  AC_DEFINE(NXCL_CYGWIN, 0, Define if compiling on Cygwin)
fi

dnl Do some platform checking for OS X

if test "$(uname)" = Darwin; then
  AC_DEFINE(NXCL_DARWIN, 1, Define if compiling on Darwin)
else
  AC_DEFINE(NXCL_DARWIN, 0, Define if compiling on Darwin)
fi

dnl Add in doc/Makefile to this:
AC_CONFIG_FILES([Makefile lib/Makefile nxcl/Makefile test/Makefile doc/Makefile nxcl.pc])

AC_OUTPUT
