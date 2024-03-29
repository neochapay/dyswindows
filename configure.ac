dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.58)

dnl OK, autoconf stuff first
AC_INIT(Y, 0.2.7, neochapay@gmail.com, Y)
AC_COPYRIGHT([Copyright 2002, Mark Thomas <markbt@efaref.net>
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

See README.license for more details])

AC_CONFIG_SRCDIR(Y/main/y.c)
AC_CONFIG_AUX_DIR(build-misc)
AC_CONFIG_MACRO_DIR(autoconf)

AC_CANONICAL_HOST

AM_INIT_AUTOMAKE(1.8 subdir-objects)
AM_MAINTAINER_MODE

AC_CONFIG_HEADERS(Y/setup.h)

orig_CFLAGS="$CFLAGS -D_GNU_SOURCE -lpthread -std=gnu99"
orig_CXXFLAGS="$CXXFLAGS -D_GNU_SOURCE -lpthread"
orig_LDFLAGS="$LDFLAGS"

AC_PROG_CXX
AM_PROG_CC_C_O
AC_ISC_POSIX

AC_PROG_LIBTOOL

dnl Concept stolen from the libtool Debian package. libtool 1.5 does something
dnl strange involving overriding the standard libraries, which is broken, and
dnl nobody can remember why it does it. So it gets disabled.

dnl This will break with libtool 1.6

sed -e 's/^\(predep_objects\)=.*/\1=""/' \
    -e 's/^\(postdep_objects\)=.*/\1=""/' \
    -e 's/^\(compiler_lib_search_path\)=.*/\1=""/' \
    -e 's:^\(sys_lib_search_path_spec\)=.*:\1="/lib/ /usr/lib/ /usr/X11R6/lib/ /usr/local/lib/":' \
    -e 's/^\(archive_cmds=\".*\) -nostdlib /\1 /' \
    -e 's/^\(archive_expsym_cmds=\".*\) -nostdlib /\1 /' \
    < libtool > libtool.gcc-indep
mv libtool.gcc-indep libtool || exit 1

AC_C_CONST
AC_C_INLINE

debug_syms=""
AC_ARG_ENABLE(debug-syms,
[  --disable-debug-syms    Do not include debug symbols in executables ],
[if test "$enableval" = "yes"
then
	debug_syms='-g '
fi], debug_syms='-g ')

AC_ARG_ENABLE(tc-syms,
[  --enable-tc-syms        Enable test coverage data ],
[if test "$enableval" = "yes"
then
	if test "$GCC" != "yes"
	then
		AC_MSG_ERROR(requires gcc)
	fi
dnl This won't work between gcc 3.1 and 3.3, and I want to profile
dnl libYc++, so it's disabled until I can use gcc 3.4.
dnl	debug_syms="${debug_syms}-ftest-coverage -fprofile-arcs "
	debug_syms="${debug_syms}-ftest-coverage "
fi])

AC_ARG_ENABLE(profile,
[  --enable-profile        Enable profiling data ],
[if test "$enableval" = "yes"
then
	if test "$GCC" != "yes"
	then
		AC_MSG_ERROR(requires gcc)
	fi
dnl This won't work between gcc 3.1 and 3.3, and I want to profile
dnl libYc++, so it's disabled until I can use gcc 3.4.
dnl	debug_syms="${debug_syms}-pg -fprofile-arcs "
	debug_syms="${debug_syms}-pg "
fi])

AC_MSG_CHECKING(for suitable optimisation flags)
AC_ARG_ENABLE(optimise,
[  --enable-optimise       Enable optimisation ],
[if test "$enableval" = "yes"
then
	if test "$GCC" = "yes"
	then
		optimise='-O2 '
	else
		optimise='-O '
	fi
else
	if test "$GCC" = "yes"
	then
		optimise='-O0 '
	else
		optimise=''
	fi
fi],[
	if test -f "$srcdir/development"
	then
		if test "$GCC" = "yes"
		then
			optimise='-O0 '
		else
			optimise=''
		fi
	else
		if test "$GCC" = "yes"
		then
			optimise='-O2 '
		else
			optimise='-O '
		fi
	fi
])
if test -z "${optimise}"
then
	AC_MSG_RESULT(none)
else
	AC_MSG_RESULT(${optimise})
fi

AC_ARG_WITH(efence, [  --with-efence           Link with electric fence ])
if test "$with_efence" = "yes"
then
	AC_CHECK_LIB(efence, EF_ALIGNMENT, LIBS="${LIBS} -lefence", AC_MSG_ERROR(libefence not found))
fi

dnl See what warnings we can get away with
C_GCC_TRY_FLAGS([-Wall])
C_GCC_TRY_FLAGS([-std=gnu99], [gcc])
C_GCC_TRY_FLAGS([-Wwrite-strings])
C_GCC_TRY_FLAGS([-Wpointer-arith])
C_GCC_TRY_FLAGS([-Wcast-qual])
C_GCC_TRY_FLAGS([-Wimplicit])
C_GCC_TRY_FLAGS([-Wnested-externs], [gcc])
C_GCC_TRY_FLAGS([-Wstrict-prototypes -Wmissing-prototypes], [gcc])
C_GCC_TRY_FLAGS([-Wmissing-declarations], [gcc])
C_GCC_TRY_FLAGS([-W -Wno-unused])
C_GCC_TRY_FLAGS([-Wundef])
C_GCC_TRY_FLAGS([-Wpacked])
C_GCC_TRY_FLAGS([-Wbad-function-cast], [gcc])
C_GCC_TRY_FLAGS([-Wfloat-equal])
C_GCC_LD_TRY_FLAGS([--warn-common])
C_GCC_LD_TRY_FLAGS([--fatal-warnings])

ACX_PTHREAD(,[AC_MSG_ERROR([DYSwindow requires a posix thread implementation to compile])])

if test x"$PTHREAD_CFLAGS" = x
then
        pthread_flags=""
else
        pthread_flags="$PTHREAD_CFLAGS "
fi
ac_save_CFLAGS="${pthread_flags}${debug_syms}${optimise}${CWARNS}${orig_CFLAGS}"
CFLAGS=${ac_save_CFLAGS}
ac_save_CXXFLAGS="${pthread_flags}${debug_syms}${optimise}${CXXWARNS}${orig_CXXFLAGS}"
CXXFLAGS=${ac_save_CXXFLAGS}
ac_save_LDFLAGS="${LDWARNS}${orig_LDFLAGS}"
LDFLAGS=${ac_save_LDFLAGS}

if test x"$PTHREAD_LIBS" != x
then
        LIBS="$PTHREAD_LIBS $LIBS"
fi

AC_PATH_PROG(PERL, perl)
AC_ARG_VAR(PERL, [Path to perl interpreter])

FREETYPE_CHECK(9.1.3,,[AC_MSG_ERROR([Y needs freetype to compile.])])

dnl LIBSDL_CHECK(1.2.0,,[AC_MSG_ERROR([Y needs libSDL to compile.])])

PKG_CHECK_MODULES(LIBSIGC, sigc++-2.0 >= 2.0)
dnl pkg-config is retarded
dnl LIBSIGC_CFLAGS=`pkg-config --libs sigc++-1.2`
dnl LIBSIGC_LIBS=`pkg-config --cflags sigc++-1.2`
LIBSIGC_CPPFLAGS="$LIBSIGC_CFLAGS"
AC_SUBST(LIBSIGC_CPPFLAGS)
AC_SUBST(LIBSIGC_LIBS)

PKG_CHECK_MODULES(LIBPNG, libpng12 >= 1.2)
AC_SUBST(LIBPNG_CFLAGS)
AC_SUBST(LIBPNG_LIBS)


dnl check for cairo
PKG_CHECK_MODULES(CAIRO, cairo >= 1.0)
CAIRO_LIBS=`pkg-config --libs cairo`
AC_SUBST(CAIRO_LIBS)
CAIRO_CPPFLAGS=`pkg-config --cflags cairo`
AC_SUBST(CAIRO_CPPFLAGS)

dnl check for glitz-glx
PKG_CHECK_MODULES(GLITZ_GLX, glitz-glx,
[
dnl check that cairo is built to support glitz
PKG_CHECK_MODULES(CAIRO_GLITZ, cairo-glitz, [have_glitz=yes], [AC_MSG_RESULT([no, disabling gl support]);have_glitz=no])
],
[AC_MSG_RESULT([no, disabling gl support]);have_glitz=no])

AC_SUBST(GLITZ_GLX_CFLAGS)
AC_SUBST(GLITZ_GLX_LIBS)

AC_ARG_ENABLE(glitz,
[  --disable-glitz         Do not build glitz-glx video backend],
[
if test "$enableval" = "yes"
then
  enable_glitz=$have_glitz
else
  enable_glitz=no
fi
],
[
enable_glitz=$have_glitz
])

AM_CONDITIONAL(WANT_GLITZ, test x$enable_glitz = xyes)



LIBITERM_CHECK

AC_ARG_ENABLE(yiterm,
[  --disable-yiterm        Do not compile yiterm],
[
if test "$enableval" = "yes"
then
  enable_yiterm=yes
else
  enable_yiterm=no
fi
],
[
enable_yiterm=$have_libiterm
])

AM_CONDITIONAL(WANT_YITERM, test x$enable_yiterm = xyes)

AC_ARG_ENABLE(trace,
[  --enable-trace          Build traceY],
[
if test "$enableval" = "yes"
then
  enable_trace=yes
else
  enable_trace=no
fi
],
[
enable_trace=no
])

AM_CONDITIONAL(WANT_TRACE, test x$enable_trace = xyes)

BINUTILS_CXX_VERSION_SCRIPT([
cxx_versioned_symbols=yes
],
[
cxx_versioned_symbols=no
])

AM_CONDITIONAL(CXX_VERSIONED_SYMBOLS, test x$cxx_versioned_symbols = xyes)

AC_CONFIG_FILES(
Makefile
Y/Makefile
libYc++/Makefile
libYconfig/Makefile
libYconfig/examples/Makefile
build-misc/Makefile
config/Makefile
config/keymaps/Makefile
data/Makefile
doc/Makefile
doc/doxygen-libYc++.cfg
doc/doxygen-Y.cfg
modules/Makefile
modules/drivers/Makefile
modules/drivers/video/Makefile
modules/drivers/video/xlib/Makefile
modules/drivers/video/glx/Makefile
modules/drivers/input/Makefile
modules/drivers/input/evdev/Makefile
modules/themes/Makefile
modules/themes/default/Makefile
modules/themes/eau/Makefile
modules/wm/Makefile
modules/wm/default/Makefile
clients/Makefile
clients/tools/Makefile
clients/tools/startY
clients/calculator/Makefile
clients/clock/Makefile
clients/sample/Makefile
clients/terminal/Makefile
startY.devel
)

AC_CONFIG_COMMANDS(chmod-startY.devel, [chmod a+x startY.devel], [])

AC_OUTPUT

echo "
Configuration:

  Detected OS:      ${host_os}
  Install prefix:   ${prefix}
  CFLAGS:           ${CFLAGS}
  CXXFLAGS:         ${CXXFLAGS}
  LDFLAGS:          ${LDFLAGS}
"

if test x$cxx_versioned_symbols = xno; then
  echo "Your linker does not appear to support C++ versioned symbols. It is probably"
  echo "either too old, or not GNU binutils."
  echo ""
  echo "A workaround has been enabled that should permit libYc++ to basically work,"
  echo "but you must avoid having two different versions of libYc++ installed at the"
  echo "same time, and binaries linked against it on this system should not be"
  echo "distributed."
fi
