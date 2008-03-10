dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.59)
AC_INIT([nxlaunch], [1.0], [seb@esfnet.co.uk])
AC_CONFIG_SRCDIR(src/main.c)
AC_REVISION([$Revision: 1.3 $])
AC_PREFIX_DEFAULT(/usr/local)

AM_INIT_AUTOMAKE([1.9 foreign])

AM_CONFIG_HEADER(config.h)
AM_MAINTAINER_MODE

dnl Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_CHECK_PROG(DOXYGEN, doxygen, doxygen, /bin/echo)
AC_CHECK_PROG(NXCLD, nxcld, nxcld, /bin/echo)

PKG_CHECK_MODULES(PACKAGE, glib-2.0 gtk+-2.0 libglade-2.0 libxml-2.0 dbus-1)

dnl Checks for library functions.
AC_HEADER_STDC
AC_HEADER_TIME
AC_CHECK_FUNCS(strftime gettimeofday uname)

dnl Determine host system type
AC_CANONICAL_HOST
AC_DEFINE_UNQUOTED(HOST, "$host", [The host system nxlaunch was configured for])

dnl pkg_modules="libxml2" <- Check this and modify when necessary
dnl PKG_CHECK_MODULES(PACKAGE, [$pkg_modules])
AC_SUBST(PACKAGE_CFLAGS)
AC_SUBST(PACKAGE_LIBS)

dnl enable/disable compiler warnings (using gcc), if specified
AC_ARG_ENABLE(warnings,
  [  --disable-warnings      disable gcc warnings [default=no]],
  [case "${enableval}" in
    yes) nxlaunch_warnings=yes ;; 
    no)  nxlaunch_warnings=no ;;
    *) AC_MSG_ERROR(bad value ${enableval} for --enable-warnings) ;;
  esac], nxlaunch_warnings=yes)
AC_MSG_CHECKING(whether to use gcc verbose warnings)
AC_MSG_RESULT($nxlaunch_warnings)
if (test "$nxlaunch_warnings" = yes && test "$GCC" = yes); then
  CFLAGS="-Wall $CFLAGS"
fi

dnl enable/disable debug logging, if specified
AC_ARG_ENABLE(debug-output,
  [  --enable-debug-output   enable debug output [default=no]],
  [case "${enableval}" in
    yes) nxlaunch_debug=yes ;;
    no)  nxlaunch_debug=no ;;
    *) AC_MSG_ERROR(bad value ${enableval} for --enable-debug-output) ;;
  esac], nxlaunch_debug=no)
AC_MSG_CHECKING(whether to enable debugging output)
AC_MSG_RESULT($nxlaunch_debug)
if test "$nxlaunch_debug" = yes; then
  AC_DEFINE(DEBUG, 1, Define if debugging output should be enabled)
else
  AC_DEFINE(DEBUG, 0, Define if debugging output should be enabled)
fi

dnl enable build of configuration feature, if required
AC_ARG_ENABLE(build-config,
  [  --enable-build-config   enable build of configuration features [default=yes]],
  [case "${enableval}" in
    yes) nxlaunch_config=yes ;;
    no)  nxlaunch_config=no ;;
    *) AC_MSG_ERROR(bad value ${enableval} for --enable-build-config) ;;
  esac], nxlaunch_config=yes)
AC_MSG_CHECKING(whether to enable the build of the configuration features in nxlaunch - most users should say yes)
AC_MSG_RESULT($nxlaunch_config)
if test "$nxlaunch_config" = yes; then
  AC_DEFINE(BUILT_IN_CONFIGURATION, 1, Define if this program should have the features to configure connections)
else
  AC_DEFINE(BUILT_IN_CONFIGURATION, 0, Define if this program should have the features to configure connections)
fi

dnl Always output certain files

AC_CONFIG_FILES([Makefile src/Makefile lib/Makefile])

AC_OUTPUT
